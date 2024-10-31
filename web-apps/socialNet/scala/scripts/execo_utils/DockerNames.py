#!/usr/bin/env python3

import os

class DockerNamesGetter :

    def __init__ (self, outputDir, nodes, client) :
        self.outputDir = outputDir
        self.client = client
        self.nodes = nodes

    def start (self):
        self.client.launchAndWaitCmd (self.nodes, "docker ps -a | tee /tmp/docker_names")

    def kill (self) :
        for n in self.nodes :
            path = self.outputDir + "/" + str (n.address)
            if (not (os.path.exists (path) and os.path.isdir (path))):
                os.makedirs (path)

            self.client.downloadFiles ([n], ["/tmp/docker_names"], path)
            result = {}
            with open (path + "/docker_names", 'r') as fp:
                i = 0
                for line in fp.readlines ():
                    if i != 0 :
                        txt = line.split ()
                        result [txt[0]] = txt [-1]
                    i += 1

            with open (path + "/docker_names", "w") as fp:
                for r in result :
                    fp.write (str (r) + ' : ' + str (result [r]) + "\n")
