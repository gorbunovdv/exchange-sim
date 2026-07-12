#!/usr/bin/env python3
import json
import sys

from ydata_profiling import ProfileReport
import pandas as pd
import argparse


def load_config(config_file):
    with open(config_file, 'r') as f:
        return json.load(f)


def main():
    parser = argparse.ArgumentParser(description='Generate a profile report for a CSV file.')
    parser.add_argument('config_file', type=str, help='Path to the configuration file')

    args = parser.parse_args()
    config = load_config(args.config_file)
    table = pd.read_json(config['calc_stats']['output_file'])
    profile = ProfileReport(table.sample(n=min(len(table), config['calc_stats']['sample_size'])), title='Massive Stats Report')
    profile.to_file(config['calc_stats']['output_file'] + '.html')



if __name__ == "__main__":
    sys.exit(main())