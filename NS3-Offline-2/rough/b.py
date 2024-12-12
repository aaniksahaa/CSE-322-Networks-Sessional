import os
import glob
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

class TCPProtocolPlotter:
    def __init__(self, base_dir, run_number):
        self.base_dir = Path(base_dir)
        self.run_number = run_number
        self.data_dir = self.base_dir / f"run-{run_number}" / "data"
        self.plots_dir = self.base_dir / f"run-{run_number}" / "plots"
        self.colors = {'TcpVeno': 'blue', 'TcpWestwoodPlus': 'red'}
        
    def parse_data_file(self, file_path):
        """Read the data file and return a pandas DataFrame with time and value columns."""
        try:
            df = pd.read_csv(file_path, delimiter=' ', header=None, names=['time', 'value'])
            return df
        except Exception as e:
            print(f"Error reading file {file_path}: {e}")
            return None

    def get_metric_name(self, filename):
        """Extract metric name from filename"""
        return filename.split('-')[-1].replace('.data', '')

    def get_flow_number(self, filename):
        """Extract flow number from filename"""
        return int(filename.split('-flow')[1].split('-')[0])

    def create_protocol_plots(self, protocol):
        """Create individual plots for a single protocol"""
        protocol_data_dir = self.data_dir / protocol
        protocol_plots_dir = self.plots_dir / protocol
        protocol_plots_dir.mkdir(parents=True, exist_ok=True)

        # Group files by flow number
        files_by_flow = {}
        for file_path in protocol_data_dir.glob("*.data"):
            if not file_path.name.endswith('.ascii'):
                flow_num = self.get_flow_number(file_path.name)
                files_by_flow.setdefault(flow_num, []).append(file_path)

        # Create plots for each flow
        for flow_num, files in sorted(files_by_flow.items()):
            flow_dir = protocol_plots_dir / f"flow{flow_num}"
            flow_dir.mkdir(exist_ok=True)

            for file_path in files:
                metric = self.get_metric_name(file_path.name)
                df = self.parse_data_file(file_path)

                if df is not None:
                    plt.figure(figsize=(10, 6))
                    plt.plot(df['time'], df['value'], '-o', 
                            color=self.colors[protocol],
                            markersize=2,
                            label=protocol)
                    plt.title(f'{protocol} Flow {flow_num} - {metric} vs Time')
                    plt.xlabel('Time (s)')
                    plt.ylabel(metric.upper())
                    plt.grid(True)
                    plt.legend()
                    
                    plot_path = flow_dir / f'{metric}.png'
                    plt.savefig(plot_path)
                    plt.close()
                    print(f"Created plot: {plot_path}")

    def create_combined_plots(self):
        """Create combined plots comparing both protocols for each metric and flow"""
        combined_dir = self.plots_dir / "combined"
        combined_dir.mkdir(parents=True, exist_ok=True)

        # Get all available protocols
        protocols = [d.name for d in self.data_dir.iterdir() if d.is_dir()]

        # Find common flows and metrics
        flow_metrics = {}
        for protocol in protocols:
            protocol_dir = self.data_dir / protocol
            for file_path in protocol_dir.glob("*.data"):
                if not file_path.name.endswith('.ascii'):
                    flow_num = self.get_flow_number(file_path.name)
                    metric = self.get_metric_name(file_path.name)
                    flow_metrics.setdefault(flow_num, set()).add(metric)

        # Create combined plots
        for flow_num, metrics in flow_metrics.items():
            flow_dir = combined_dir / f"flow{flow_num}"
            flow_dir.mkdir(exist_ok=True)

            for metric in metrics:
                plt.figure(figsize=(10, 6))
                
                for protocol in protocols:
                    file_path = self.data_dir / protocol / f"TcpVariantsComparison-flow{flow_num}-{metric}.data"
                    if file_path.exists():
                        df = self.parse_data_file(file_path)
                        if df is not None:
                            plt.plot(df['time'], df['value'], '-o',
                                   color=self.colors[protocol],
                                   markersize=2,
                                   label=protocol)

                plt.title(f'Flow {flow_num} - {metric} Comparison')
                plt.xlabel('Time (s)')
                plt.ylabel(metric.upper())
                plt.grid(True)
                plt.legend()
                
                plot_path = flow_dir / f'{metric}_comparison.png'
                plt.savefig(plot_path)
                plt.close()
                print(f"Created combined plot: {plot_path}")

    def generate_all_plots(self):
        """Generate all individual and combined plots"""
        # Create individual protocol plots
        for protocol in ['TcpVeno', 'TcpWestwoodPlus']:
            self.create_protocol_plots(protocol)
        
        # Create combined comparison plots
        self.create_combined_plots()

def main():
    # Example usage
    base_dir = "./results"  # Adjust this path as needed
    run_number = 2  # Adjust run number as needed
    
    plotter = TCPProtocolPlotter(base_dir, run_number)
    plotter.generate_all_plots()

if __name__ == "__main__":
    main()