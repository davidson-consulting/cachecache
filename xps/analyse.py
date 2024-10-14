import argparse
from matplotlib import pyplot as plt
import csv
import glob
from pathlib import Path
import numpy as np

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

def parse_csv_list(file):
    data = {}
    with open(file, "r") as csvfile:
        reader = csv.DictReader(csvfile, delimiter=';')
        for row in reader:
            client = row['client']
            if client not in data:
                data[client] = {}

            time = float(row['time'])
            if time not in data[client]:
                data[client][time] = {}

            for k, v in row.items():
                if k in ['time', 'client']:
                    continue

                if k not in data[client][time]:
                    data[client][time][k] = []
                data[client][time][k].append(v)

    return data


def plot_percentiles(input_file, output_file):
    clients = {} 

    with open(input_file, "r") as csvfile:
        reader = csv.DictReader(csvfile, delimiter=';')
        for row in reader:
            client = row['client']
            time = float(row['time'])
            percentage = float(row['percentage'])
            percentile = float(row['percentile'])

            if client not in clients:
                clients[client] = {}

            if percentage not in clients[client]:
                clients[client][percentage] = {'times': [time], 'percentiles': [percentile]}
            else:
                clients[client][percentage]['times'].append(time)
                clients[client][percentage]['percentiles'].append(percentile)

    plt.clf()
    for client in clients:
        for percentage in clients[client]:
            plt.plot(clients[client][percentage]['times'], clients[client][percentage]['percentiles'], label=f"{client} - {percentage}p")

    plt.legend()
    plt.savefig(output_file)


def plot_nb_reqs(input_file, output_file):
    data = parse_csv(input_file)

    plt.clf()
    for client, rows in data.items():
        time = list(rows.keys())
        size = []
        for t in time:
            size.append(int(rows[t]['nb_reqs']))
        plt.plot(time, size, label=client)
    plt.title('Nb of requests')
    plt.legend()
    plt.savefig(output_file)


def plot_cache_size(input_file, output_file):
    data = parse_csv(input_file)

    plt.clf()
    for client, rows in data.items():
        time = list(rows.keys())
        size = []
        for t in time:
            size.append(int(rows[t]['cache_size']))
        plt.plot(time, size, label=client)
        #plt.plot(time, [100 * 1024 * 1024 for _ in range(len(time))], label=f"requested - {client}")
    plt.title('Cache size')
    plt.legend()
    plt.savefig(output_file)

def plot_memory_usage(input_file, output_file):
    data = parse_csv(input_file)

    plt.clf()
    for client, rows in data.items():
        time = list(rows.keys())
        size = []
        for t in time:
            size.append(int(rows[t]['memory_usage']))
        plt.plot(time, size, label=client)
        #plt.plot(time, [100 * 1024 * 1024 for _ in range(len(time))], label=f"requested - {client}")
    plt.title('Memory usage')
    plt.legend()
    plt.savefig(output_file)

def plot_wallet_usage_diff(input_file, output_file):
    data = parse_csv(input_file)

    plt.clf()
    for client, rows in data.items():
        time = list(rows.keys())
        size = []
        for t in range(1, len(time)):
            size.append(int(rows[time[t]]['wallet']) - int(rows[time[t-1]]['wallet']))
        plt.plot(time[1:], size, label=client)
    plt.title('Wallet (diff between t and t-1)')
    plt.legend()
    plt.savefig(output_file)


def plot_hit_ratio(input_hits, input_reqs, output_file):
    data_hits = parse_csv(input_hits)
    data_reqs = parse_csv(input_reqs)

    plt.clf() 
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
        plt.plot(time[1000:len(time)], ratio_avg, label=client)
    plt.title("Hit ratio")
    plt.legend()
    plt.savefig(output_file)

def plot_hits(input_hits, output_file):
    data_hits = parse_csv(input_hits)

    plt.clf() 
    for client, rows in data_hits.items():
        time = list(rows.keys())
        hits = []

        for t in time:
            hits.append(int(rows[t]['hits']))

        plt.plot(time, hits, label=client)
    plt.title("# hits")
    plt.legend()
    plt.savefig(output_file)

def plot_wallets(input_file, output_file):
    data = parse_csv(input_file)
    
    plt.clf()
    for client, rows in data.items():
        time = list(rows.keys())
        wallets = []

        for t in time:
            wallets.append(int(rows[t]['wallet']))
        plt.plot(time, wallets, label=client)
    plt.title("Wallet")
    plt.legend()
    plt.savefig(output_file)

def plot_nb_evictions(input_file, output_file):
    data = parse_csv(input_file)
    
    plt.clf()
    for client, rows in data.items():
        time = list(rows.keys())
        wallets = []

        for t in time:
            wallets.append(int(rows[t]['nb_evictions']))
        plt.plot(time, wallets, label=client)
    plt.title("Number of evictions")
    plt.legend()
    plt.savefig(output_file)

def plot_size_evictions(input_file, output_file):
    data = parse_csv(input_file)
    
    plt.clf()
    for client, rows in data.items():
        time = list(rows.keys())
        wallets = []

        for t in time:
            wallets.append(int(rows[t]['size_eviction']))
        plt.plot(time, wallets, label=client)
    plt.title("Size of evictions")
    plt.legend()
    plt.savefig(output_file)

def plot_eviction_target(input_file, output_file):
    data = parse_csv(input_file)
    
    plt.clf()
    for client, rows in data.items():
        time = list(rows.keys())
        wallets = []

        for t in time:
            wallets.append(float(rows[t]['eviction_target']))
        plt.plot(time, wallets, label=client)
    plt.title("Eviction target")
    plt.legend()
    plt.savefig(output_file)

def plot_delta(input_file, output_file):
    data = parse_csv_list(input_file)
    
    plt.clf()
    for client, rows in data.items():
        time = list(rows.keys())
        deltas = []

        for t in time:
            d = list(map(lambda x: int(x), rows[t]['delta']))
            d = sum(d) / len(d)
            deltas.append(d)
        plt.plot(time, deltas, label=client)
    plt.title("Time between two calls")
    plt.legend()
    plt.savefig(output_file)

def analyze(input_directory, output_directory):
    plot_hit_ratio(f"{input_directory}/hits.csv", f"{input_directory}/nb_reqs.csv", f"{output_directory}/hit_ratio.png")
    #plot_wallets(f"{input_directory}/wallet.csv", f"{output_directory}/wallet.png")
    plot_hits(f"{input_directory}/hits.csv", f"{output_directory}/hits.png")
    plot_cache_size(f"{input_directory}/cache_size.csv", f"{output_directory}/cache_size.png")
    plot_memory_usage(f"{input_directory}/memory_usage.csv", f"{output_directory}/memory_usage.png")
    #plot_wallet_usage_diff(f"{input_directory}/wallet.csv", f"{output_directory}/wallet_diff.png")
    plot_nb_evictions(f"{input_directory}/nb_evictions.csv", f"{output_directory}/nb_evictions.png")
    plot_size_evictions(f"{input_directory}/size_eviction.csv", f"{output_directory}/size_eviction.png")
    plot_eviction_target(f"{input_directory}/eviction_target.csv", f"{output_directory}/eviction_target.png")
    plot_percentiles(f"{input_directory}/percentile.csv", f"{output_directory}/percentiles.png")
    plot_delta(f"{input_directory}/delta.csv", f"{output_directory}/deltas.png")


if __name__ == "__main__":
    parser = argparse.ArgumentParser("analyze")
    parser.add_argument("input_directory", help="Directory CSV files")
    parser.add_argument("output_directory", help="Where to save figures")
    args = parser.parse_args()
    
    analyze(args.input_directory, args.output_directory)
