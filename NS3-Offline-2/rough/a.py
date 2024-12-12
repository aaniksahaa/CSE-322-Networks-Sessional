import os
import glob
import argparse
import matplotlib.pyplot as plt
import pandas as pd
from pathlib import Path
from collections import defaultdict

def parse_data_file(file_path):
    """Read the data file and return a pandas DataFrame with time and value columns."""
    try:
        df = pd.read_csv(file_path, delimiter=' ', header=None, names=['time', 'value'])
        return df
    except Exception as e:
        print(f"Error reading file {file_path}: {e}")
        return None

def get_metric_name(filename):
    """Extract metric name from filename (e.g., 'rtt' from 'TcpVariantsComparison-flow0-rtt.data')"""
    return filename.split('-')[-1].replace('.data', '')

def get_flow_number(filename):
    """Extract flow number from filename (e.g., '0' from 'TcpVariantsComparison-flow0-rtt.data')"""
    return int(filename.split('-flow')[1].split('-')[0])

def create_plots(data_dir, output_dir, min_flow=None, max_flow=None):
    """
    Create line plots for TCP data files, organized by flow and metric.
    
    Args:
        data_dir: Directory containing the data files
        output_dir: Directory to save the plots
        min_flow: Minimum flow number to process (inclusive)
        max_flow: Maximum flow number to process (inclusive)
    """
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    # Get all .data files
    data_files = glob.glob(os.path.join(data_dir, "*.data"))
    
    # Group files by flow number
    flow_files = {}
    for file_path in data_files:
        filename = os.path.basename(file_path)
        if filename.endswith('.ascii'):
            continue
            
        flow_num = get_flow_number(filename)
        
        # Skip if flow number is outside specified range
        if min_flow is not None and flow_num < min_flow:
            continue
        if max_flow is not None and flow_num > max_flow:
            continue
            
        flow_files.setdefault(flow_num, []).append(file_path)
    
    # Process each flow
    for flow_num, files in sorted(flow_files.items()):
        # Create flow directory
        flow_dir = os.path.join(output_dir, f"flow{flow_num}")
        os.makedirs(flow_dir, exist_ok=True)
        
        # Process each metric file for this flow
        for file_path in files:
            metric = get_metric_name(os.path.basename(file_path))
            df = parse_data_file(file_path)
            
            if df is not None:
                # Create the plot
                plt.figure(figsize=(10, 6))
                plt.plot(df['time'], df['value'], '-o', markersize=2)
                plt.title(f'Flow {flow_num} - {metric} vs Time')
                plt.xlabel('Time')
                plt.ylabel(metric.upper())
                plt.grid(True)
                
                # Save the plot
                plot_path = os.path.join(flow_dir, f'{metric}.png')
                plt.savefig(plot_path)
                plt.close()
                print(f"Created plot: {plot_path}")

def main():
    # parser = argparse.ArgumentParser(description='Generate TCP data plots')
    # parser.add_argument('--data-dir', required=True, help='Directory containing the data files')
    # parser.add_argument('--output-dir', required=True, help='Directory to save the plots')
    # parser.add_argument('--min-flow', type=int, help='Minimum flow number to process (inclusive)')
    # parser.add_argument('--max-flow', type=int, help='Maximum flow number to process (inclusive)')
    
    # args = parser.parse_args()

    # create_plots(args.data_dir, args.output_dir, args.min_flow, args.max_flow)

    data_dir = "results"
    output_dir = "plots"
    min_flow = 0
    max_flow = 13
    
    create_plots(data_dir, output_dir, min_flow, max_flow)

if __name__ == "__main__":
    main()