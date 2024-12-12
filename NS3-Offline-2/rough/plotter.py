import os
import glob
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
import matplotlib.colors as mcolors

# Configure flow range here
FLOW_START = 0  # inclusive
FLOW_END = 6    # inclusive

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
        self.colors = {'TcpVeno': 'blue', 'TcpWestwoodPlus': 'red', 'TcpLogWestwoodPlus': 'green'}
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


    def create_combined_plots(self):
        """Create combined plots comparing protocols"""
        combined_dir = self.plots_dir / "combined"
        combined_dir.mkdir(parents=True, exist_ok=True)
        
        # Define protocol pairs for comparison
        protocol_comparisons = [
            # All protocols
            ['TcpVeno', 'TcpWestwoodPlus', 'TcpLogWestwoodPlus'],
            # Veno and WestwoodPlus
            ['TcpVeno', 'TcpWestwoodPlus'],
            # WestwoodPlus and LogWestwoodPlus
            ['TcpWestwoodPlus', 'TcpLogWestwoodPlus']
        ]
        
        # Collect all metrics and data
        metrics_data = {}  # Structure: {protocol: {metric: {flow_num: df}}}
        all_protocols = ['TcpVeno', 'TcpWestwoodPlus', 'TcpLogWestwoodPlus']
        
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

            # Create per-flow comparison plots
            for flow_num in range(FLOW_START, FLOW_END + 1):
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

    # def create_combined_plots(self):
    #     """Create combined plots comparing protocols"""
    #     combined_dir = self.plots_dir / "combined"
    #     combined_dir.mkdir(parents=True, exist_ok=True)
        
    #     protocols = [d.name for d in self.data_dir.iterdir() if d.is_dir()]
        
    #     # Collect all metrics and data
    #     metrics_data = {}  # Structure: {protocol: {metric: {flow_num: df}}}
    #     for protocol in protocols:
    #         protocol_dir = self.data_dir / protocol
    #         for file_path in protocol_dir.glob("*.data"):
    #             if not file_path.name.endswith('.ascii'):
    #                 flow_num = self.get_flow_number(file_path.name)
    #                 if flow_num is None or flow_num < FLOW_START or flow_num > FLOW_END:
    #                     continue
                        
    #                 metric = self.get_metric_name(file_path.name)
    #                 if metric is None:
    #                     continue
                        
    #                 df = self.parse_data_file(file_path)
    #                 if df is not None and not df.empty:
    #                     metrics_data.setdefault(protocol, {}).setdefault(metric, {})[flow_num] = df

    #     # Get all unique metrics
    #     all_metrics = set()
    #     for protocol_data in metrics_data.values():
    #         all_metrics.update(protocol_data.keys())

    #     # Create per-flow comparison plots
    #     for flow_num in range(FLOW_START, FLOW_END + 1):
    #         flow_dir = combined_dir / f"flow{flow_num}"
    #         flow_dir.mkdir(exist_ok=True)

    #         for metric in all_metrics:
    #             # Create normal scale plot
    #             plt.figure(figsize=(10, 6))
    #             valid_data = False
                
    #             for protocol in protocols:
    #                 if (protocol in metrics_data and 
    #                     metric in metrics_data[protocol] and 
    #                     flow_num in metrics_data[protocol][metric]):
    #                     df = metrics_data[protocol][metric][flow_num]
    #                     plt.plot(df['time'], df['value'], '-o',
    #                            color=self.colors[protocol],
    #                            markersize=2,
    #                            label=protocol)
    #                     valid_data = True

    #             if valid_data:
    #                 plt.title(f'Flow {flow_num} - {metric} Comparison')
    #                 plt.xlabel('Time (s)')
    #                 plt.ylabel(metric.upper())
    #                 plt.grid(True)
    #                 plt.legend()
    #                 self.save_plot(plt, flow_dir, f'{metric}_comparison')

    #                 # Create additional log scale plot for ssth
    #                 if metric == 'ssth':
    #                     plt.figure(figsize=(10, 6))
    #                     for protocol in protocols:
    #                         if (protocol in metrics_data and 
    #                             metric in metrics_data[protocol] and 
    #                             flow_num in metrics_data[protocol][metric]):
    #                             df = metrics_data[protocol][metric][flow_num]
    #                             plt.plot(df['time'], df['value'], '-o',
    #                                    color=self.colors[protocol],
    #                                    markersize=2,
    #                                    label=protocol)
                        
    #                     plt.yscale('log')
    #                     plt.title(f'Flow {flow_num} - {metric} Comparison (Log Scale)')
    #                     plt.xlabel('Time (s)')
    #                     plt.ylabel(metric.upper())
    #                     plt.grid(True)
    #                     plt.legend()
    #                     self.save_plot(plt, flow_dir, f'{metric}_comparison', "_log")

    #     # Create combined flow comparison plots
    #     combined_flow_dir = combined_dir / "flowcombined"
    #     combined_flow_dir.mkdir(exist_ok=True)
        
    #     for metric in all_metrics:
    #         # Create normal scale plot
    #         plt.figure(figsize=(12, 7))
    #         valid_data = False
            
    #         for protocol in protocols:
    #             if protocol in metrics_data and metric in metrics_data[protocol]:
    #                 for flow_num, df in sorted(metrics_data[protocol][metric].items()):
    #                     color_idx = (flow_num * len(protocols) + list(protocols).index(protocol)) % len(self.flow_colors)
    #                     plt.plot(df['time'], df['value'], '-o',
    #                             color=self.flow_colors[color_idx],
    #                             markersize=2,
    #                             label=f'{protocol} Flow {flow_num}')
    #                     valid_data = True
            
    #         if valid_data:
    #             plt.title(f'{metric} Comparison - All Flows')
    #             plt.xlabel('Time (s)')
    #             plt.ylabel(metric.upper())
    #             plt.grid(True)
    #             plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    #             plt.tight_layout()
    #             self.save_plot(plt, combined_flow_dir, f'{metric}_all_flows_comparison')

    #             # Create additional log scale plot for ssth
    #             if metric == 'ssth':
    #                 plt.figure(figsize=(12, 7))
    #                 for protocol in protocols:
    #                     if protocol in metrics_data and metric in metrics_data[protocol]:
    #                         for flow_num, df in sorted(metrics_data[protocol][metric].items()):
    #                             color_idx = (flow_num * len(protocols) + list(protocols).index(protocol)) % len(self.flow_colors)
    #                             plt.plot(df['time'], df['value'], '-o',
    #                                     color=self.flow_colors[color_idx],
    #                                     markersize=2,
    #                                     label=f'{protocol} Flow {flow_num}')
                    
    #                 plt.yscale('log')
    #                 plt.title(f'{metric} Comparison - All Flows (Log Scale)')
    #                 plt.xlabel('Time (s)')
    #                 plt.ylabel(metric.upper())
    #                 plt.grid(True)
    #                 plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    #                 plt.tight_layout()
    #                 self.save_plot(plt, combined_flow_dir, f'{metric}_all_flows_comparison', "_log")

    def generate_all_plots(self):
        """Generate all individual and combined plots"""
        print(f"Generating plots for flows {FLOW_START} to {FLOW_END}")
        
        for protocol in ['TcpVeno', 'TcpWestwoodPlus', 'TcpLogWestwoodPlus']:
            print(f"\nProcessing {protocol}...")
            self.create_protocol_plots(protocol)
        
        print("\nCreating combined plots...")
        self.create_combined_plots()

def main():
    base_dir = "./results"  # Adjust this path as needed
    run_number = 3  # Adjust run number as needed
    
    plotter = TCPProtocolPlotter(base_dir, run_number)
    plotter.generate_all_plots()

if __name__ == "__main__":
    main()