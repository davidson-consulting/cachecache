#!/usr/bin/env python3

#!/usr/bin/env python3

import csv
import matplotlib.pylab as plt
import os
import argparse
import numpy
from scipy.signal import lfilter


def readCsv (filename) :
    #
    # Read a csv file, and return the header and each rows
    # @params:
    #    - filename: a csv file
    #
    #
    with open (filename, "r") as fp :
        reader = csv.reader (fp)
        head = next (reader)[0].split (';')
        rows = []
        for row in reader :
            rows.append (row[0].split (';'))
        return (head, rows)

def parseEnergyCsv (anon):
    #
    # Parse an energy csv file
    # @returns: the consumption in watt (one point per second), and the speeds of the CPU cores in hz
    #
    (head, rows) = readCsv (anon)
    nb_cores = len (head) - 4
    energy = []
    speeds = []
    for row in rows :
        val = float (row[1].strip ())
        energy.append (val)
        speeds.append ([])
        for i in range (nb_cores) :
            speeds [-1].append (int (row [i + 4]))

    watts = [energy [0]]
    for i in range (1, len (energy)) :
        watts.append (energy [i] - energy [i - 1])

    return (watts, speeds)

def parseCgroupNames (anon):
    (head, rows) = anon
    names = []
    for row in rows :
        if (row [1] not in names ):
            names += [row [1]]
    return names

def parsePerfCountersCsv (anon, extract, doSum = False) :
    #
    # Parse a perf counter csv file
    # @params:
    #    - filtering: only get the line where filtering is contained in the cgroup name
    #    - exluding: get the lines where exluding is no contained in the cgroup name
    #
    # @returns: The sum of performance counter (one point per second)
    #
    (head, rows) = anon
    result = {}
    test = False
    for row in rows :
        if row [1] == extract or (doSum and row [1] != "#SYSTEM"):
            time = int (float (row [0].strip ()) * 1000)
            for i in range (2, len (head)) :
                Divider = 1
                if ("MEMORY" in head [i]) :
                    Divider = 1024 * 1024 * 1024
                elif "CPU_CLOCK" in head [i] :
                    Divider = 1000 * 1000 * 1000

                if head [i] in result :
                    if (time in result [head [i]]):
                        result [head [i]][time] += float (row [i].strip ()) / Divider
                    else :
                        result [head [i]][time] = float (row [i].strip ()) / Divider
                else :
                    result [head [i]] = {time : float (row [i].strip ()) / Divider}
    RR = {}
    for i in result :
        lists = sorted(result [i].items()) # sorted by key, return a list of tuples
        x_2, y_2 = zip(*lists)
        RR [i] = y_2

    return RR
