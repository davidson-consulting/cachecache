#!/usr/bin/env python3

from execo import *
from execo_g5k import *
import urllib.request
import time
import logging
from subprocess import Popen, PIPE

DockerConfig = """
version: '3.8'
services:
  textservice-mysql:
    image: mysql:8.0.30
    volumes:
      - ./config/mysql:/etc/mysql/conf.d
    environment:
      - MYSQL_ALLOW_EMPTY_PASSWORD=yes
    # If you want to expose these ports outside your dev PC,
    ports:
      - {{PORT}}:3306
    command: mysqld --lower_case_table_names=1 --skip-ssl --character_set_server=utf8mb4 --explicit_defaults_for_timestamp
"""

class MysqlDatabaseDeployer :

    # ****************************
    # Create a deployer for the registry
    # ****************************
    def __init__ (self, client, node, name, ips, config) :
        self.client = client
        self.ips = ips
        self.node = node
        self.name = name
        self.config = config
        print (node)

    # *****
    # Start the mysql database on the node
    # *****
    def start (self) :
        config = DockerConfig.replace ("{{PORT}}", str (self.config ["port"]))
        with open ("/tmp/" + self.name + ".yaml", 'w') as fp:
            fp.write (config)

        self.client.uploadFiles ([self.node], ["/tmp/" + self.name + ".yaml"], "/tmp/SocialNetworkAKKA/" + self.name + ".yaml")
        self.kill ()
        self.client.launchAndWaitCmd ([self.node], "docker-compose -f /tmp/SocialNetworkAKKA/" + self.name + ".yaml up -d")


    def fill (self) :
        if ("scripts" in self.config):
            for f in self.config ["scripts"] :
                script = "../resources/datas/" + f + ".sql"
                self.client.uploadFiles ([self.node], [script], "/tmp/SocialNetworkAKKA/" + f + ".sql")
                self.client.launchAndWaitCmd ([self.node], "mysql -h " + self.ips [self.name] + " -u root -P " + str (self.config ["port"]) + " " + f + " < /tmp/SocialNetworkAKKA/" + f + ".sql")
        if ("dump" in self.config) :
            script = "../resources/datas" + self.config ["dump"] + ".sql"
            self.client.uploadFiles ([self.node], [script], "/tmp/SocialNetworkAKKA/" + self.config ["dump"] + ".sql")
            self.client.launchAndWaitCmd ([self.node], "mysql -h " + self.ips [self.name] + " -u root -P " + str (self.config ["port"]) + " < /tmp/SocialNetworkAKKA/" + self.config ["dump"] + ".sql")

    # *****
    # Kill the mysql database on the node
    # *****
    def kill (self):
        self.client.launchAndWaitCmd ([self.node], "docker-compose -f /tmp/SocialNetworkAKKA/" + self.name + ".yaml down")
