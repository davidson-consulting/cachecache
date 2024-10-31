#!/usr/bin/env python3

from execo import *
from execo_g5k import *
import urllib.request
import time
import logging
import os


DockerConfig = """
version: '3.8'
services:
  {{NAME}}-redis:
    image: redis:6.2.7
    # If you want to expose these ports outside your dev PC,
    # remove the "127.0.0.1:" prefix
    ports:
      - {{PORT}}:6379
"""
class RedisCacheDeployer :

    # ****************************
    # Create a deployer for the registry
    # ****************************
    def __init__ (self, outputDir, client, node, name, config) :
        self.outputDir = outputDir
        self.client = client
        self.node = node
        self.name = name
        self.config = config
        self.monitorCmd = None

    # *****
    # Start the mysql database on the node
    # *****
    def start (self) :
        config = DockerConfig.replace ("{{PORT}}", str (self.config ["port"]))
        config = config.replace ("{{NAME}}", self.name)

        with open ("/tmp/" + self.name + "_loc.yaml", 'w') as fp:
            fp.write (config)

        self.client.uploadFiles ([self.node], ["/tmp/" + self.name + "_loc.yaml"], "/tmp/SocialNetworkAKKA/" + self.name + ".yaml")
        self.client.launchAndWaitCmd ([self.node], "docker-compose -f /tmp/SocialNetworkAKKA/" + self.name + ".yaml up -d")

        if ("size" in self.config) :
            self.client.launchAndWaitCmd ([self.node], "sudo apt-get install -y redis-tools")
            port = self.config ["port"]
            size = self.config ["size"]

            self.client.launchAndWaitCmd ([self.node], f"redis-cli -p {port} config set maxmemory {size}")
            self.client.launchAndWaitCmd ([self.node], f"redis-cli -p {port} config set maxmemory-policy allkeys-lru")

        if ("eviction" in self.config) :
            eviction = self.config ["eviction"]
            self.client.launchAndWaitCmd ([self.node], f"redis-cli -p {port} config set maxmemory-policy {eviction}")

        self.client.uploadFiles ([self.node], ["../resources/utils/redis-monitor.py"], "/tmp/SocialNetworkAKKA/monitor-" + self.name + ".py")
        self.monitorCmd = self.client.launchCmd ([self.node], "cd /tmp/SocialNetworkAKKA ; python3 monitor-" + self.name + ".py " + str (self.config ["port"]) + " logs/monitor-" + self.name + ".csv")

    # *****
    # Kill the mysql database on the node
    # *****
    def kill (self):
        self.monitorCmd.kill ()
        path = self.outputDir + "/" + str (self.node.address) + "/redis/"
        if (not (os.path.exists (path) and os.path.isdir (path))):
            os.makedirs (path)

        self.client.downloadFiles ([self.node], ["/tmp/SocialNetworkAKKA/logs/monitor-" + self.name + ".csv"], path)
        self.client.launchAndWaitCmd ([self.node], "docker-compose -f /tmp/SocialNetworkAKKA/" + self.name + ".yaml down")
