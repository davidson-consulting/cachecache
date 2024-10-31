#!/usr/bin/env python3

import csv
import matplotlib.pylab as plt
import os
import argparse
import numpy
from scipy.signal import lfilter
import math
from os import walk
from analyse_utils import loading


def parseArguments ():
    parser = argparse.ArgumentParser ()
    parser.add_argument ("dir")

    return parser.parse_args ()


def findDockerName (dir, name):
    with open (dir + "/docker_names", 'r') as fp:
        for line in fp.readlines ():
            txt = line.split ()
            if (txt [0] in name) :
                return txt [-1][len ("socialnetwork_"):]
    return name


def readRedisLogs (path):
    w = walk(path + "/redis")
    files = []
    for (dirpath, dirnames, filenames) in w:
        files = filenames
        break

    for f in files :
        hits = []
        misses = []
        ratio = []
        memory = []

        (head, rows) = loading.readCsv (path + "/redis/" + f)
        for values in rows:
            hits = hits + [int (values[0])]
            misses = misses + [int (values[1])]
            if ((hits [-1] + misses [-1]) != 0):
                ratio = ratio + [misses [-1] / (hits [-1] + misses [-1]) * 10]
            else :
                ratio = ratio + [0]

            memory = memory + [float (values [2]) / 1000]

        #plt.clf ()
        #plt.plot ([0 for i in range (15)] + ratio, label="redis-ratio-" + str (f))

        # plt.clf ()
        plt.plot ([0 for i in range (15)] + memory, label="redis-memory" + str (f))
        # plt.savefig ("./.out/" + f + ".memory.jpg")


def main (args):
    if (not os.path.isdir ("./.out")):
        os.mkdir ("./.out")

    host = args.dir + "/energy.csv"
    cgrps = args.dir + "/cgroups.csv"

    csv = loading.readCsv (cgrps)

    (RAPL, speeds) = loading.parseEnergyCsv (host)
    NAMES = loading.parseCgroupNames (csv)
    allPerfs = {}
    for i in NAMES :
        if (i != "#SYSTEM"):
            allPerfs [i] = loading.parsePerfCountersCsv (csv, extract=i)
        if "docker-" in i :
            name = findDockerName (args.dir, i)
            allPerfs [name] = allPerfs [i]
            del allPerfs [i]
    allPerfs ["SYSTEM"] = loading.parsePerfCountersCsv (csv, "", doSum=True)

    (head, rows) = csv
    for i in head [2:] :
        for name in allPerfs :
            # plt.clf ()

            if ("PERF_COUNT" in i and ("SYSTEM" in name)
                #or "mysql" in name or "registry" in name or "front" in name or "all-in-one" in name or
                #"redis" in name)
                                       ):
                plt.plot (allPerfs [name][i], label="MEM_" + name)
            pass

            # plt.savefig ("./.out/" + i + "_" + name.split ("/")[-1] + ".jpg")

    #plt.clf ()
    plt.plot (RAPL[165:280], label="RAPL")
    print (sum (RAPL [165:280]))
    #plt.savefig ("./.out/RAPL.jpg")

    readRedisLogs (args.dir)
    plt.legend ()
    plt.show ()# ("out.jpg")

if __name__ == "__main__":
    main (parseArguments ())
