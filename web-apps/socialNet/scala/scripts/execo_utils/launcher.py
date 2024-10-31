
from execo import *
from execo_g5k import *
import yaml
import logging
import sys
import os
import time
import json

import subprocess
from os import walk
from pathlib import Path

# ***********************************************
# This class is responsible to node connection, and VM creations
# It possesses nodes, and VMs connections
# ***********************************************
class ExecoClient :

    # *****************************
    # @params:
    #    - nodes: the list of host nodes (execo nodes)
    # @info: this constructor should only be called from self.fromG5K, or self.fromIps
    # *****************************
    def __init__ (self, nodes) :
        self._hnodes = nodes

    # *****************************
    # Create an execo client from ips
    # @params:
    #   - ips: the list of node ips
    #   - user: the user of those machines
    # *****************************
    @classmethod
    def fromIps (cls, ips = ["127.0.0.1"], user = ["root"], keyfile = os.path.expanduser ('~') + "/.ssh/id_rsa") :
        nodes = []
        nodeNames = {}
        for i in range (0, len (ips)) :
            nodes = nodes + [Host (ips [i], user=user [i], keyfile=keyfile)]
            nodeNames [ips [i]] = nodes [-1]

        return (cls (nodes), nodeNames)


    # *****************************
    # Configure the nodes to be able to execute the services
    # *****************************
    def configureNodes (self, withInstall = True, withDownload = True) :
        if (withInstall) :
            pass

        self.launchAndWaitCmd (self._hnodes, "cd /tmp/ ; rm -rf /tmp/SocialNetworkAKKA ; mkdir -p /tmp/SocialNetworkAKKA/resources ; mkdir -p /tmp/SocialNetworkAKKA/logs")

    # ================================================================================
    # ================================================================================
    # =========================        SERVICE UTILS         =========================
    # ================================================================================
    # ================================================================================

    # ****************************
    # Upload the jar to the remote node
    # @params:
    #    - nodes: the list of nodes
    # ****************************
    def uploadJar (self, nodes, user = "root") :
        conn_params = {'user': user}
        cmd = Put (nodes, ["../target/scala-3.2.2/socialnetwork-assembly-1.0.jar"], "/tmp/SocialNetworkAKKA/app.jar", connection_params = conn_params)
        cmd.run ()
        cmd.wait ()

    # ================================================================================
    # ================================================================================
    # =========================        COMMAND UTILS         =========================
    # ================================================================================
    # ================================================================================

    # ************************************
    # Run a bash command on remote nodes, and waits its completion
    # @params:
    #   - nodes: the list of nodes
    #   - cmd: the command to run
    #   - user: the user that runs the command
    # ************************************
    def launchAndWaitCmd (self, nodes, cmd, user = "root") :
        conn_params = {'user': user}
        cmd_run = Remote (cmd, nodes, conn_params)
        logger.info ("Launch " + cmd + " on " + str (nodes) + " " + str (conn_params))
        cmd_run.start ()
        status = cmd_run.wait ()
        logger.info ("Done")

    # ************************************
    # Run a bash command on remote nodes, but do not wait its completion
    # @params:
    #   - nodes: the list of nodes
    #   - cmd: the command to run
    #   - user: the user that runs the command
    # ************************************
    def launchCmd (self, nodes, cmd, user = "root") :
        conn_params = {'user': user}
        cmd_run = Remote (cmd, nodes, conn_params)
        logger.info ("Launch " + cmd + " on " + str (nodes) + " " + str (conn_params))
        cmd_run.start ()
        return cmd_run

    # ************************************
    # Kill commands before their completion
    # @params:
    #    - cmds: the list of command to kill
    # ************************************
    def killCmds (self, cmds):
        for c in cmds:
            c.kill ()

    # ************************************
    # Wait the completion of commands (do not restart failing commands)
    # @params:
    #    - cmds: the list of command to wait
    # ************************************
    def waitCmds (self, cmds) :
        for c in cmds :
            c.wait ()

    # ************************************
    # Wait the completion of commands, and restart them if they failed for some reason
    # @params:
    #    - cmds: the list of command to wait
    # ************************************
    def waitAndForceCmds (self, cmds) :
        while True :
            restart = []
            for c in cmds :
                status = c.wait (timeout = get_seconds (0.1))
                if not status.ok :
                    logger.info ("Command failed, restarting : " +  str (c))
                    c.reset ()
                    c.start ()
                    restart = restart + [c]
                elif not status.finished_ok :
                    restart = restart + [c]
                else :
                    logger.info ("Command finished " + str (c))
            if restart != []:
                cmds = restart
            else :
                break

    # ************************************
    # Download image files on remote nodes
    # @params:
    #   - nodes: the list of nodes
    #   - images: the list of images to download (wget)
    #   - user: the user that will download the images
    # ************************************
    def downloadImages (self, nodes, images, user = "root") :
        for i in images :
            cmd = "wget " + images [i] + " -O .qcow2/" + i + ".qcow2"
            self.launchAndWaitCmd (nodes, cmd)

        logger.info ("Done")

    # ****************************
    # Upload files located on localhost to remote nodes
    # @params:
    #    - nodes: the list of nodes
    #    - files: the list of files to upload
    #    - directory: where to put the files
    #    - user: the user on the remote nodes
    # ****************************
    def uploadFiles (self, nodes, files, directory,  user = "root") :
        conn_params = {'user': user}
        cmd_run = Put (nodes, files, directory, connection_params=conn_params)
        logger.info ("Upload files on " + str (nodes) + ":" + str (files))
        cmd_run.run ()
        cmd_run.wait ()
        logger.info ("Done")

    # ****************************
    # Download files located on remote nodes to localhost
    # @params:
    #    - nodes: the list of nodes
    #    - files: the list of files to download
    #    - directory: where to put the files
    #    - user: the user on the remote nodes
    # ****************************
    def downloadFiles (self, nodes, files, directory, user = "root") :
        conn_params = {'user': user}
        cmd_run = Get (nodes, files, directory, connection_params=conn_params)
        logger.info (f"Download files on {nodes} {files}")
        cmd_run.run ()
        cmd_run.wait ()
        logger.info ("Done")
