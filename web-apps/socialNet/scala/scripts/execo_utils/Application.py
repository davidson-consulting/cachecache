#!/usr/bin/env python3

#!/usr/bin/env python3

from execo import *
from execo_g5k import *
import urllib.request
import time
import logging

import os

from execo_utils import launcher
from execo_utils import MysqlDatabase
from execo_utils import RedisCache
from execo_utils import Vjoule
from execo_utils import DockerNames

import yaml

class ApplicationDeployer :

    def __init__ (self, configPath, outputDir) :
        with open (configPath, 'r') as fp :
            self.serviceConfig = yaml.load (fp, Loader=yaml.FullLoader)
            self.databaseConfig = self.serviceConfig ["dbs"]
            if ("caches" in self.serviceConfig) :
                self.cacheConfig = self.serviceConfig ["caches"]
            else:
                self.cacheConfig = {}

            self.nodeConfig = self.serviceConfig ["nodes"]
            self.actorConfig = self.serviceConfig ["actors"]
            self.registryConfig = self.serviceConfig ["registry"]
            self.frontConfig = self.serviceConfig ["front"]

        (client, nodes, actorHost, ips, alls) = self._listNodes (self.nodeConfig, self.actorConfig, self.registryConfig, self.frontConfig, self.databaseConfig, self.cacheConfig)

        self.client = client
        self.nodes = nodes
        self.ips = ips
        self.outputDir = outputDir
        self.actorHost = actorHost

        self.databases = self._findDatabases (self.databaseConfig)
        self.caches = self._findCaches (self.cacheConfig)
        self.vjoule = Vjoule.VJouleServiceDeployer (self.outputDir, alls.values (), self.client)
        self.dock = DockerNames.DockerNamesGetter (self.outputDir, alls.values (), self.client)

        self.registryCmd = None
        self.frontCmd = None
        self.actorCmd = []

    # ==========
    # Start the deployement of the application
    # ==========
    def start (self):
        self.client.configureNodes ()
        self.vjoule.start ()

        for i in self.databases :
            self.databases [i].start ()

        for i in self.caches :
            self.caches [i].start ()

        self.dock.start ()

        self.client.uploadJar (self.nodes)
        self._launchRegistry ()
        self._launchFront ()

        time.sleep (15) # 15 seconds for the dbs to be available
        for actor in self.actorConfig :
            self._launchActor (actor, self.actorConfig [actor])
        time.sleep (5)

    def _launchActor (self, actorName, actor) :
        self._generateActorConfig (self.registryConfig, actor, "/tmp/socialNet", actorName)
        self.client.uploadFiles (self.actorHost [actorName], [f"/tmp/socialNet/{actorName}.yaml"], "/tmp/SocialNetworkAKKA/")
        self.client.uploadFiles (self.actorHost [actorName], ["../resources/master.conf"], "/tmp/SocialNetworkAKKA/resources/")
        cmd = self.client.launchCmd ([self.actorHost [actorName]], f"sudo cgcreate -g memory,cpu:SocialNetworkAKKA/{actorName}")
        cmd.wait ()

        self.actorCmd = self.actorCmd + [self.client.launchCmd ([self.actorHost [actorName]], f"cd /tmp/SocialNetworkAKKA ; sudo cgexec -g memory,cpu:SocialNetworkAKKA/{actorName} java -cp app.jar com.socialnet.master.main.Main {actorName}.yaml | tee logs/{actorName}.log")]


    def _generateActorConfig (self, registryConfig, config, where, actorName):
        yamlRes = {}
        yamlRes ["addr"] = self.ips [actorName]
        yamlRes ["port"] = config["port"]
        yamlRes ["registry"] = { "port" : registryConfig ["port"], "addr" : self.ips ["#registry"] }
        yamlRes ["services"] = config ["services"]

        yamlRes ["databases"] = {}
        yamlRes ["caches"] = {}
        for i in config ["services"] :
            db = config ["services"][i]["db"]
            if (db != "none" and db not in yamlRes ["databases"]):
                yamlRes ["databases"][db] = {"type" : "mysql",
                                             "addr" : self.ips [db],
                                             "port" : self.databaseConfig [db]["port"] }
            if ("cache" in config ["services"][i]):
                cache = config ["services"][i]["cache"]
                if (cache != "none" and cache not in yamlRes ["caches"]):
                    yamlRes ["caches"][cache] = {"type" : "redis",
                                                 "addr" : self.ips [cache],
                                                 "port" : self.cacheConfig [cache]["port"]}

            if ("rqt-cache" in config ["services"][i]):
                cache = config ["services"][i]["rqt-cache"]
                if (cache != "none" and cache not in yamlRes ["caches"]):
                    yamlRes ["caches"][cache] = {"type" : "redis",
                                                 "addr" : self.ips [cache],
                                                 "port" : self.cacheConfig [cache]["port"]}


        if not os.path.exists (where) or not os.path.isdir (where):
            os.makedirs (where)

        with open (str (where + f"/{actorName}.yaml"), 'w') as fp:
            fp.write (yaml.dump (yamlRes))

    def _launchFront (self) :
        self._generateFrontConfig (self.registryConfig, self.frontConfig, "/tmp/socialNet")
        self.client.uploadFiles (self.actorHost ["#front"], ["/tmp/socialNet/front.yaml"], "/tmp/SocialNetworkAKKA/")
        self.client.uploadFiles (self.actorHost ["#front"], ["../resources/master.conf"], "/tmp/SocialNetworkAKKA/resources/")
        cmd = self.client.launchCmd ([self.actorHost ["#front"]], "sudo cgcreate -g memory,cpu:SocialNetworkAKKA/front")
        cmd.wait ()

        self.frontCmd = self.client.launchCmd ([self.actorHost ["#front"]], "cd /tmp/SocialNetworkAKKA ; sudo cgexec -g memory,cpu:SocialNetworkAKKA/front java -cp app.jar com.socialnet.front.main.FrontApp front.yaml | tee logs/front.log")
        time.sleep (0.1)

    def _generateFrontConfig (self, registryConf, config, where) :
        yamlRes = {}
        yamlRes ["addr"] = self.ips ["#front"]
        yamlRes ["httpPort"] = config["httpPort"]
        yamlRes ["port"] = config["actorPort"]
        yamlRes ["registry"] = { "port" : registryConf ["port"], "addr" : self.ips ["#registry"] }

        if not os.path.exists (where) or not os.path.isdir (where):
            os.makedirs (where)

        with open (str (where + "/front.yaml"), 'w') as fp:
            fp.write (yaml.dump (yamlRes))

    def _launchRegistry (self):
        self._generateRegistryConfig (self.registryConfig, "/tmp/socialNet/")
        self.client.uploadFiles (self.actorHost ["#registry"], ["/tmp/socialNet/registry.yaml"], "/tmp/SocialNetworkAKKA/")
        self.client.uploadFiles (self.actorHost ["#registry"], ["../resources/registry.conf"], "/tmp/SocialNetworkAKKA/resources/")
        cmd = self.client.launchCmd ([self.actorHost ["#registry"]], "sudo cgcreate -g memory,cpu:SocialNetworkAKKA/registry")
        cmd.wait ()

        self.registryCmd = self.client.launchCmd ([self.actorHost ["#registry"]], "cd /tmp/SocialNetworkAKKA ; sudo cgexec -g memory,cpu:SocialNetworkAKKA/registry java -cp app.jar com.socialnet.registry.main.Main registry.yaml | tee logs/registry.log")
        time.sleep (0.1)

    def _generateRegistryConfig (self, config, where) :
        yamlRes = {}
        yamlRes ["port"] = config["port"]
        yamlRes ["addr"] = self.ips ["#registry"]
        if not os.path.exists (where) or not os.path.isdir (where):
            os.makedirs (where)

        with open (str (where + "/registry.yaml"), 'w') as fp:
            fp.write (yaml.dump (yamlRes))

    # ==========
    # Launch the insertion scripts
    # ==========
    def insertDatas (self):
        for i in self.databases :
            self.databases [i].fill ()

    # ==========
    # Kill all running services
    # ==========
    def kill (self) :
        self.frontCmd.kill ()
        for a in self.actorCmd :
            a.kill ()

        self.registryCmd.kill ()

        self.vjoule.kill ()
        self.dock.kill ()

        for i in self.databases :
            self.databases [i].kill ()

        for i in self.caches :
            self.caches [i].kill ()

    # ****************************
    # List the nodes that are used to run service, and connect to them
    # @returns: the client used to communicate with the nodes, and the list of nodes for each service
    # ****************************
    def _listNodes (self, nodeConfig, actorConfig, registryConfig, frontConfig, databaseConfig, cacheConfig) :
        alls = {}
        actorHost = {}
        databases = {}
        ips = {}
        users = {}

        for node in nodeConfig :
            alls [node] = nodeConfig [node]["addr"]
            users [node] = nodeConfig [node]["user"]

        (client, nodes) = launcher.ExecoClient.fromIps (ips = list (alls.values ()), user=list (users.values ()))
        for actor in actorConfig :
            if ("node" in actorConfig [actor]) :
                actorHost [actor] = nodes [alls [actorConfig [actor]["node"]]]
                ips [actor] = alls [actorConfig [actor]["node"]]

        if ("node" in registryConfig):
            actorHost ["#registry"] = nodes [alls [registryConfig ["node"]]]
            ips ["#registry"] = alls [registryConfig ["node"]]

        if ("node" in frontConfig):
            actorHost ["#front"] = nodes [alls [frontConfig ["node"]]]
            ips ["#front"] = alls [frontConfig ["node"]]

        for db in databaseConfig :
            if ("node" in databaseConfig [db]) :
                actorHost [db] = nodes [alls [databaseConfig [db]["node"]]]
                ips [db] = alls [databaseConfig [db]["node"]]

        for ch in cacheConfig :
            if ("node" in cacheConfig [ch]) :
                actorHost [ch] = nodes [alls [cacheConfig [ch]["node"]]]
                ips [ch] = alls [cacheConfig [ch]["node"]]

        return (client, nodes, actorHost, ips, nodes)


    def _findCaches (self, cacheConfig) :
        caches = {}
        for cache in cacheConfig :
            if ("type" in cacheConfig [cache] and cacheConfig [cache]["type"] == "redis") :
                caches [cache] = RedisCache.RedisCacheDeployer (self.outputDir, self.client, self.actorHost [cache], cache, cacheConfig [cache])
        return caches

    def _findDatabases (self, databaseConfig) :
        databases = {}
        for database in databaseConfig :
            if ("type" in databaseConfig [database] and databaseConfig [database]["type"] == "mysql") :
                databases [database] = MysqlDatabase.MysqlDatabaseDeployer (self.client, self.actorHost [database], database, self.ips, databaseConfig [database])
        return databases
