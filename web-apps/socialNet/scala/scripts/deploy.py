#!/usr/bin/env python3

from execo_utils import launcher
from execo_utils import Application

from execo import *
from execo_g5k import *
import logging
import yaml
import time
import argparse
import subprocess

import signal
import sys


def signal_handler(sig, frame):
    logger.info ("Killing every services")

def waitQuit ():
    signal.signal(signal.SIGINT, signal_handler)
    logger.info ('Wait until Ctrl+C is pressed')
    signal.pause()

def parseArguments ():
    parser = argparse.ArgumentParser ()
    parser.add_argument ("config", help="the configuration file")
    parser.add_argument ("--out", help="the output directory, default is /tmp/social-network-results", default="/tmp/social-network-results")

    return parser.parse_args ()

def main (args):
    app = Application.ApplicationDeployer (args.config, args.out)
    app.start ()
    app.insertDatas ()

    waitQuit ()
    app.kill ()

if __name__ == "__main__" :
    main (parseArguments ())
