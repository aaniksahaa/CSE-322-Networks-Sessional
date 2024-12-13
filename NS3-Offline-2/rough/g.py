import os
import glob
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
import matplotlib.colors as mcolors
from scipy.interpolate import interp1d
import numpy as np

RUN_NUMBER = 4

# Configure flow range here
FLOW_START = 0  # inclusive
FLOW_END = 6    # inclusive

PLOT_ONLY_ONE_SINGLE_FLOW = True
PLOT_INDIVIDUAL_PROTOCOLS = False 

PROTOCOLS_TO_PLOT = [
                     'TcpVeno', 
                     'TcpWestwoodPlus', 
                     'TcpLogWestwoodPlus', 
                     'TcpStochasticLogWestwoodPlus'
                     ]
PROTOCOL_COMPARISONS = [
    ['TcpVeno', 'TcpWestwoodPlus', 'TcpLogWestwoodPlus'],
    ['TcpVeno', 'TcpWestwoodPlus'],
    ['TcpWestwoodPlus', 'TcpLogWestwoodPlus'],
    ['TcpLogWestwoodPlus', 'TcpStochasticLogWestwoodPlus']
]

def create_plot(plt, df, title, xlabel, ylabel, color, label, is_log=False):
    """Helper function to create a plot with optional log scale"""
    if is_log:
        plt.yscale('log')
        title += ' (Log Scale)'
    plt.plot(df['time'], df['value'], '-o', 
            color=color,
            markersize=2,
            label=label)
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.grid(True)
    plt.legend()

class TCPProtocolPlotter:
    def __init__(self, base_dir, run_number):
        self.base_dir = Path(base_dir)
        self.run_number = run_number
        self.data_dir = self.base_dir / f"run-{run_number}" / "data"
        self.plots_dir = self.base_dir / f"run-{run_number}" / "plots"
        self.colors = {'TcpVeno': 'blue', 'TcpWestwoodPlus': 'red', 'TcpLogWestwoodPlus': 'green', 'TcpStochasticLogWestwoodPlus': 'navy'}
        self.flow_colors = list(mcolors.TABLEAU_COLORS.values())

    def parse_data_file(self, file_path):
        """Read the data file and return a pandas DataFrame with time and value columns."""
        try:
            if os.path.getsize(file_path) == 0:
                print(f"Warning: Empty file {file_path}")
                return None
                
            df = pd.read_csv(file_path, delimiter=' ', header=None, names=['time', 'value'])
            
            print(f"Read file {file_path}, shape: {df.shape}")
            if df.shape[0] == 0:
                print(f"Warning: No data in {file_path}")
                return None
                
            return df
        except Exception as e:
            print(f"Error reading file {file_path}: {e}")
            return None

    def get_metric_name(self, filename):
        """Extract metric name from filename properly handling hyphenated names"""
        parts = filename.split('-')
        for i, part in enumerate(parts):
            if part.startswith('flow'):
                metric = '-'.join(parts[i+1:]).replace('.data', '')
                return metric
        return None

    def get_flow_number(self, filename):
        """Extract flow number from filename"""
        parts = filename.split('-')
        for part in parts:
            if part.startswith('flow'):
                return int(part[4:])
        return None

    def save_plot(self, plt, base_path, metric, suffix=""):
        """Helper function to save plot with proper naming"""
        plot_path = base_path / f'{metric}{suffix}.png'
        plt.savefig(plot_path, bbox_inches='tight')
        plt.close()
        print(f"Created plot: {plot_path}")

    def create_protocol_plots(self, protocol):
        """Create individual plots for a single protocol"""
        protocol_data_dir = self.data_dir / protocol
        protocol_plots_dir = self.plots_dir / protocol
        protocol_plots_dir.mkdir(parents=True, exist_ok=True)

        # Group files by flow number and collect metrics data
        files_by_flow = {}
        metrics_data = {}
        
        for file_path in protocol_data_dir.glob("*.data"):
            if not file_path.name.endswith('.ascii'):
                flow_num = self.get_flow_number(file_path.name)
                if flow_num is None or flow_num < FLOW_START or flow_num > FLOW_END:
                    continue

                metric = self.get_metric_name(file_path.name)
                if metric is None:
                    continue

                df = self.parse_data_file(file_path)
                if df is not None and not df.empty:
                    files_by_flow.setdefault(flow_num, []).append(file_path)
                    metrics_data.setdefault(metric, {})[flow_num] = df

        # Create individual flow plots
        for flow_num, files in sorted(files_by_flow.items()):
            flow_dir = protocol_plots_dir / f"flow{flow_num}"
            flow_dir.mkdir(exist_ok=True)

            for file_path in files:
                metric = self.get_metric_name(file_path.name)
                df = self.parse_data_file(file_path)

                if df is not None and not df.empty:
                    # Create normal scale plot
                    plt.figure(figsize=(10, 6))
                    title = f'{protocol} Flow {flow_num} - {metric} vs Time'
                    create_plot(plt, df, title, 'Time (s)', metric.upper(),
                              self.colors[protocol], protocol)
                    self.save_plot(plt, flow_dir, metric)

                    # Create additional log scale plot for ssth
                    if metric == 'ssth':
                        plt.figure(figsize=(10, 6))
                        create_plot(plt, df, title, 'Time (s)', metric.upper(),
                                  self.colors[protocol], protocol, is_log=True)
                        self.save_plot(plt, flow_dir, metric, "_log")

        # Create combined flow plots
        combined_flow_dir = protocol_plots_dir / "flowcombined"
        combined_flow_dir.mkdir(exist_ok=True)
        
        for metric, flows_data in metrics_data.items():
            # Create normal scale plot
            plt.figure(figsize=(12, 7))
            for flow_num, df in sorted(flows_data.items()):
                color_idx = flow_num % len(self.flow_colors)
                plt.plot(df['time'], df['value'], '-o',
                        color=self.flow_colors[color_idx],
                        markersize=2,
                        label=f'Flow {flow_num}')
            
            plt.title(f'{protocol} - {metric} vs Time (All Flows)')
            plt.xlabel('Time (s)')
            plt.ylabel(metric.upper())
            plt.grid(True)
            plt.legend()
            self.save_plot(plt, combined_flow_dir, f'{metric}_all_flows')

            # Create additional log scale plot for ssth
            if metric == 'ssth':
                plt.figure(figsize=(12, 7))
                for flow_num, df in sorted(flows_data.items()):
                    color_idx = flow_num % len(self.flow_colors)
                    plt.plot(df['time'], df['value'], '-o',
                            color=self.flow_colors[color_idx],
                            markersize=2,
                            label=f'Flow {flow_num}')
                
                plt.yscale('log')
                plt.title(f'{protocol} - {metric} vs Time (All Flows) (Log Scale)')
                plt.xlabel('Time (s)')
                plt.ylabel(metric.upper())
                plt.grid(True)
                plt.legend()
                self.save_plot(plt, combined_flow_dir, f'{metric}_all_flows', "_log")


    def create_averaged_flow_plots(self, metrics_data, output_dir, protocols):
        print('\n\nhere', protocols, '\n\n')

        """Create plots with flows averaged over time for each metric"""
        output_dir.mkdir(exist_ok=True)
        
        # Create uniform time grid (0 to 100 with 0.1 step)
        time_grid = np.arange(0, 100.1, 0.1)
        
        for metric in metrics_data:
            plt.figure(figsize=(12, 7))
            valid_data = False
            
            protocol_averages = {}
            
            for protocol in protocols:
                if protocol in metrics_data and metric in metrics_data[protocol]:
                    # Collect all flow data for this protocol and metric
                    flows_data = metrics_data[protocol][metric]
                    
                    # Initialize array to store interpolated values for each flow
                    interpolated_values = []
                    
                    for flow_num, df in flows_data.items():
                        # Create interpolation function for this flow
                        # Use 'previous' interpolation to maintain step-like behavior
                        f = interp1d(df['time'], df['value'], 
                                kind='previous', 
                                bounds_error=False,
                                fill_value=(df['value'].iloc[0], df['value'].iloc[-1]))
                        
                        # Interpolate values at uniform time grid
                        interp_values = f(time_grid)
                        interpolated_values.append(interp_values)
                    
                    if interpolated_values:
                        # Convert to numpy array for easier averaging
                        interpolated_values = np.array(interpolated_values)
                        
                        # Calculate mean and standard deviation across flows
                        mean_values = np.mean(interpolated_values, axis=0)
                        std_values = np.std(interpolated_values, axis=0)
                        
                        # Plot mean line
                        plt.plot(time_grid, mean_values, '-',
                                color=self.colors[protocol],
                                label=f'{protocol} (Mean)',
                                linewidth=2)
                        
                        # Add shaded area for standard deviation
                        plt.fill_between(time_grid,
                                    mean_values - std_values,
                                    mean_values + std_values,
                                    color=self.colors[protocol],
                                    alpha=0.2)
                        
                        # Store for potential further use
                        protocol_averages[protocol] = {
                            'time': time_grid,
                            'mean': mean_values,
                            'std': std_values
                        }
                        
                        valid_data = True
            
            if valid_data:
                plt.title(f'{metric} - Average Across Flows')
                plt.xlabel('Time (s)')
                plt.ylabel(metric.upper())
                plt.grid(True)
                plt.legend()
                
                # Save normal scale plot
                self.save_plot(plt, output_dir, f'{metric}_flow_averaged')
                
                # Create log scale version for ssth
                if metric == 'ssth':
                    plt.figure(figsize=(12, 7))
                    
                    for protocol in protocols:
                        if protocol in protocol_averages:
                            data = protocol_averages[protocol]
                            plt.plot(data['time'], data['mean'], '-',
                                    color=self.colors[protocol],
                                    label=f'{protocol} (Mean)',
                                    linewidth=2)
                            plt.fill_between(data['time'],
                                        data['mean'] - data['std'],
                                        data['mean'] + data['std'],
                                        color=self.colors[protocol],
                                        alpha=0.2)
                    
                    plt.yscale('log')
                    plt.title(f'{metric} - Average Across Flows (Log Scale)')
                    plt.xlabel('Time (s)')
                    plt.ylabel(metric.upper())
                    plt.grid(True)
                    plt.legend()
                    self.save_plot(plt, output_dir, f'{metric}_flow_averaged', "_log")

    def create_averaged_flow_plots2(self, metrics_data, output_dir, protocols):
        """Create plots with flows averaged over time for each metric"""
        print(f"\nCreating averaged flow plots for protocols: {protocols}")
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # Get all unique metrics across all protocols
        all_metrics = set()
        for protocol in protocols:
            if protocol in metrics_data:
                print(f"Metrics found for {protocol}: {list(metrics_data[protocol].keys())}")
                all_metrics.update(metrics_data[protocol].keys())
        
        # Create uniform time grid (0 to 100 with 0.1 step)
        time_grid = np.arange(0, 100.1, 0.1)
        
        for metric in all_metrics:
            print(f"\nProcessing metric: {metric}")
            plt.figure(figsize=(12, 7))
            valid_data = False
            
            protocol_averages = {}
            
            for protocol in protocols:
                print(f"Processing protocol: {protocol}")
                if protocol not in metrics_data:
                    print(f"No data found for protocol: {protocol}")
                    continue
                    
                if metric not in metrics_data[protocol]:
                    print(f"No {metric} data found for protocol: {protocol}")
                    continue
                
                # Get all flows data for this protocol and metric
                flows_data = metrics_data[protocol][metric]
                print(f"Found {len(flows_data)} flows for {protocol} {metric}")
                
                if not flows_data:
                    continue
                
                # Initialize array to store interpolated values for each flow
                interpolated_values = []
                
                # Process each flow
                for flow_num, df in flows_data.items():
                    if df.empty:
                        print(f"Empty dataframe for flow {flow_num}")
                        continue
                    
                    try:
                        # Ensure time values are within reasonable range
                        max_time = df['time'].max()
                        valid_time_grid = time_grid[time_grid <= max_time]
                        
                        if len(valid_time_grid) == 0:
                            print(f"No valid time points for flow {flow_num}")
                            continue
                        
                        # Create interpolation function
                        f = interp1d(df['time'], df['value'],
                                kind='previous',
                                bounds_error=False,
                                fill_value=(df['value'].iloc[0], df['value'].iloc[-1]))
                        
                        # Interpolate values
                        interp_values = f(valid_time_grid)
                        interpolated_values.append(interp_values)
                        print(f"Successfully interpolated flow {flow_num}")
                        
                    except Exception as e:
                        print(f"Error processing flow {flow_num}: {str(e)}")
                        continue
                
                if interpolated_values:
                    try:
                        # Pad shorter arrays to match the longest one
                        max_len = max(len(arr) for arr in interpolated_values)
                        padded_values = []
                        for arr in interpolated_values:
                            if len(arr) < max_len:
                                padded = np.pad(arr, (0, max_len - len(arr)), 'edge')
                                padded_values.append(padded)
                            else:
                                padded_values.append(arr)
                        
                        # Convert to numpy array and calculate statistics
                        interpolated_values = np.array(padded_values)
                        mean_values = np.mean(interpolated_values, axis=0)
                        std_values = np.std(interpolated_values, axis=0)
                        
                        # Plot mean line
                        time_plot = time_grid[:len(mean_values)]
                        plt.plot(time_plot, mean_values, '-',
                                color=self.colors[protocol],
                                label=f'{protocol} (Mean)',
                                linewidth=2)
                        
                        # Add shaded area for standard deviation
                        # plt.fill_between(time_plot,
                        #             mean_values - std_values,
                        #             mean_values + std_values,
                        #             color=self.colors[protocol],
                        #             alpha=0.2)
                        
                        protocol_averages[protocol] = {
                            'time': time_plot,
                            'mean': mean_values,
                            'std': std_values
                        }
                        
                        valid_data = True
                        print(f"Successfully plotted averaged data for {protocol}")
                        
                    except Exception as e:
                        print(f"Error creating plot for {protocol}: {str(e)}")
                        continue
            
            if valid_data:
                try:
                    plt.title(f'{metric} - Average Across Flows')
                    plt.xlabel('Time (s)')
                    plt.ylabel(metric.upper())
                    plt.grid(True)
                    plt.legend()
                    
                    # Save normal scale plot
                    self.save_plot(plt, output_dir, f'{metric}_flow_averaged')
                    
                    # Create log scale version for ssth
                    if metric == 'ssth':
                        plt.figure(figsize=(12, 7))
                        
                        for protocol in protocols:
                            if protocol in protocol_averages:
                                data = protocol_averages[protocol]
                                plt.plot(data['time'], data['mean'], '-',
                                    color=self.colors[protocol],
                                    label=f'{protocol} (Mean)',
                                    linewidth=2)
                                # plt.fill_between(data['time'],
                                #             data['mean'] - data['std'],
                                #             data['mean'] + data['std'],
                                #             color=self.colors[protocol],
                                #             alpha=0.2)
                        
                        plt.yscale('log')
                        plt.title(f'{metric} - Average Across Flows (Log Scale)')
                        plt.xlabel('Time (s)')
                        plt.ylabel(metric.upper())
                        plt.grid(True)
                        plt.legend()
                        self.save_plot(plt, output_dir, f'{metric}_flow_averaged', "_log")
                    
                    print(f"Successfully saved plots for {metric}")
                    
                except Exception as e:
                    print(f"Error saving plots for {metric}: {str(e)}")
            else:
                print(f"No valid data to plot for {metric}")
                plt.close()

    def create_combined_plots(self):
        """Create combined plots comparing protocols"""
        combined_dir = self.plots_dir / "combined"
        combined_dir.mkdir(parents=True, exist_ok=True)
        
        # Define protocol pairs for comparison
        protocol_comparisons = PROTOCOL_COMPARISONS
        
        # Collect all metrics and data
        metrics_data = {}  # Structure: {protocol: {metric: {flow_num: df}}}
        all_protocols = PROTOCOLS_TO_PLOT
        
        for protocol in all_protocols:
            protocol_dir = self.data_dir / protocol
            for file_path in protocol_dir.glob("*.data"):
                if not file_path.name.endswith('.ascii'):
                    flow_num = self.get_flow_number(file_path.name)
                    if flow_num is None or flow_num < FLOW_START or flow_num > FLOW_END:
                        continue
                        
                    metric = self.get_metric_name(file_path.name)
                    if metric is None:
                        continue
                        
                    df = self.parse_data_file(file_path)
                    if df is not None and not df.empty:
                        metrics_data.setdefault(protocol, {}).setdefault(metric, {})[flow_num] = df

        # Get all unique metrics
        all_metrics = set()
        for protocol_data in metrics_data.values():
            all_metrics.update(protocol_data.keys())

        # Create comparison plots for each protocol combination
        for protocols in protocol_comparisons:
            # Create directory for this protocol combination
            combo_name = "-".join(p.replace('Tcp', '') for p in protocols)
            combo_dir = combined_dir / combo_name
            combo_dir.mkdir(exist_ok=True)

            PLOT_LAST = FLOW_START if PLOT_ONLY_ONE_SINGLE_FLOW else FLOW_END
            # Create per-flow comparison plots
            for flow_num in range(FLOW_START, PLOT_LAST + 1):
                flow_dir = combo_dir / f"flow{flow_num}"
                flow_dir.mkdir(exist_ok=True)

                for metric in all_metrics:
                    # Create normal scale plot
                    plt.figure(figsize=(10, 6))
                    valid_data = False
                    
                    for protocol in protocols:
                        if (protocol in metrics_data and 
                            metric in metrics_data[protocol] and 
                            flow_num in metrics_data[protocol][metric]):
                            df = metrics_data[protocol][metric][flow_num]
                            plt.plot(df['time'], df['value'], '-o',
                                color=self.colors[protocol],
                                markersize=2,
                                label=protocol)
                            valid_data = True

                    if valid_data:
                        plt.title(f'Flow {flow_num} - {metric} Comparison')
                        plt.xlabel('Time (s)')
                        plt.ylabel(metric.upper())
                        plt.grid(True)
                        plt.legend()
                        self.save_plot(plt, flow_dir, f'{metric}_comparison')

                        # Create additional log scale plot for ssth
                        if metric == 'ssth':
                            plt.figure(figsize=(10, 6))
                            for protocol in protocols:
                                if (protocol in metrics_data and 
                                    metric in metrics_data[protocol] and 
                                    flow_num in metrics_data[protocol][metric]):
                                    df = metrics_data[protocol][metric][flow_num]
                                    plt.plot(df['time'], df['value'], '-o',
                                        color=self.colors[protocol],
                                        markersize=2,
                                        label=protocol)
                            
                            plt.yscale('log')
                            plt.title(f'Flow {flow_num} - {metric} Comparison (Log Scale)')
                            plt.xlabel('Time (s)')
                            plt.ylabel(metric.upper())
                            plt.grid(True)
                            plt.legend()
                            self.save_plot(plt, flow_dir, f'{metric}_comparison', "_log")

            # Create combined flow comparison plots
            combined_flow_dir = combo_dir / "flowcombined"
            combined_flow_dir.mkdir(exist_ok=True)
            
            for metric in all_metrics:
                # Create normal scale plot
                plt.figure(figsize=(12, 7))
                valid_data = False
                
                for protocol in protocols:
                    if protocol in metrics_data and metric in metrics_data[protocol]:
                        for flow_num, df in sorted(metrics_data[protocol][metric].items()):
                            color_idx = (flow_num * len(protocols) + list(protocols).index(protocol)) % len(self.flow_colors)
                            plt.plot(df['time'], df['value'], '-o',
                                    color=self.flow_colors[color_idx],
                                    markersize=2,
                                    label=f'{protocol} Flow {flow_num}')
                            valid_data = True
                
                if valid_data:
                    plt.title(f'{metric} Comparison - All Flows')
                    plt.xlabel('Time (s)')
                    plt.ylabel(metric.upper())
                    plt.grid(True)
                    plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
                    plt.tight_layout()
                    self.save_plot(plt, combined_flow_dir, f'{metric}_all_flows_comparison')

                    # Create additional log scale plot for ssth
                    if metric == 'ssth':
                        plt.figure(figsize=(12, 7))
                        for protocol in protocols:
                            if protocol in metrics_data and metric in metrics_data[protocol]:
                                for flow_num, df in sorted(metrics_data[protocol][metric].items()):
                                    color_idx = (flow_num * len(protocols) + list(protocols).index(protocol)) % len(self.flow_colors)
                                    plt.plot(df['time'], df['value'], '-o',
                                            color=self.flow_colors[color_idx],
                                            markersize=2,
                                            label=f'{protocol} Flow {flow_num}')
                        
                        plt.yscale('log')
                        plt.title(f'{metric} Comparison - All Flows (Log Scale)')
                        plt.xlabel('Time (s)')
                        plt.ylabel(metric.upper())
                        plt.grid(True)
                        plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
                        plt.tight_layout()
                        self.save_plot(plt, combined_flow_dir, f'{metric}_all_flows_comparison', "_log")

        # Create averaged flow plots for each protocol combination
        for protocols in protocol_comparisons:
            # Create directory for this protocol combination
            combo_name = "-".join(p.replace('Tcp', '') for p in protocols)
            combo_dir = combined_dir / combo_name
            
            # Create averaged plots directory
            averaged_dir = combo_dir / "flowaveraged"
            self.create_averaged_flow_plots2(metrics_data, averaged_dir, protocols)


    def generate_all_plots(self):
        """Generate all individual and combined plots"""
        print(f"Generating plots for flows {FLOW_START} to {FLOW_END}")
        
        if PLOT_INDIVIDUAL_PROTOCOLS:
            for protocol in PROTOCOLS_TO_PLOT:
                print(f"\nProcessing {protocol}...")
                self.create_protocol_plots(protocol)
        
        print("\nCreating combined plots...")
        self.create_combined_plots()

def main():
    base_dir = "./results"  # Adjust this path as needed
    run_number = RUN_NUMBER  # Adjust run number as needed
    
    plotter = TCPProtocolPlotter(base_dir, run_number)
    plotter.generate_all_plots()

if __name__ == "__main__":
    main()