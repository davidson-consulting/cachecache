import argparse
from matplotlib import pyplot as plt
import csv
import glob
from pathlib import Path
import numpy as np
import os

def dirs(directory):
    return [x[0] for x in os.walk(directory)][1:]

def parse_csv(file):
    data = {}
    with open(file, "r") as csvfile:
        reader = csv.DictReader(csvfile, delimiter=';')
        for row in reader:
            client = row['client']
            if client not in data:
                data[client] = {}
            if row['time'] != '':
                data[client][float(row['time'])] = {k:v for k, v in row.items() if k not in ['time', 'client']}

    return data

def _plot_hit_ratio(input_hits, input_reqs, name):
    data_hits = parse_csv(input_hits)
    data_reqs = parse_csv(input_reqs)

    for client, rows in data_hits.items():
        time = list(rows.keys())
        hits = []
        for t in time:
            hits.append(int(rows[t]['hits']))

        time = list(data_reqs[client].keys())
        reqs = []
        for t in time:
            reqs.append(int(data_reqs[client][t]['nb_reqs']))

        ratio = [hits[i] / reqs[i] * 100 for i in range(min(len(hits), len(reqs)))]
        ratio_avg = [sum(ratio[i-1000:i]) / 1000 for i in range(1000, len(ratio))]
        plt.plot(time[1000:len(time)], ratio_avg, label=f"{client}_{name}")


def plot_hits(input_1, input_2, output_directory):
    plt.clf()
    _plot_hit_ratio(f"{input_1}/hits.csv", f"{input_1}/nb_reqs.csv", "one")
    _plot_hit_ratio(f"{input_2}/hits.csv", f"{input_2}/nb_reqs.csv", "two")
    plt.title("Hit ratio")
    plt.legend()
    plt.savefig(f"{output_directory}/hit_ratio.png")

def _plot_memory_usage(input_file, name):
    data = parse_csv(input_file)

    for client, rows in data.items():
        time = list(rows.keys())
        size = []
        for t in time:
            size.append(int(rows[t]['memory_usage']))
        plt.plot(time, size, label=f"{client}_{name}")

def plot_memory_usage(input_1, input_2, output_directory):
    plt.clf()
    _plot_memory_usage(f"{input_1}/memory_usage.csv", "one")
    _plot_memory_usage(f"{input_2}/memory_usage.csv", "two")
    plt.title("Memory usage")
    plt.legend()
    plt.savefig(f"{output_directory}/memory_usage.png")


def analyze(input_1, input_2, output_directory):
    plot_hits(input_1, input_2, output_directory)
    plot_memory_usage(input_1, input_2, output_directory)

if __name__ == "__main__":
    parser = argparse.ArgumentParser("analyze")
    parser.add_argument("input_1", help="Directory CSV files")
    parser.add_argument("input_2", help="Directory CSV files")
    parser.add_argument("output_directory", help="Where to save figures")
    args = parser.parse_args()
    
    analyze(args.input_1, args.input_2, args.output_directory)
