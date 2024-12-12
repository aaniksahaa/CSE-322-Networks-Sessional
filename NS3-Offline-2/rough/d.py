import os
import glob
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

RUN_NUMBER = 3

# Configure flow range here
FLOW_START = 0  # inclusive
FLOW_END = 2    # inclusive

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
            # Skip empty files
            if os.path.getsize(file_path) == 0:
                print(f"Warning: Empty file {file_path}")
                return None
                
            df = pd.read_csv(file_path, delimiter=' ', header=None, names=['time', 'value'])
            
            # Debug print
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
        # Split by '-' and get all parts after 'flowN'
        parts = filename.split('-')
        for i, part in enumerate(parts):
            if part.startswith('flow'):
                # Join all remaining parts except the .data extension
                metric = '-'.join(parts[i+1:]).replace('.data', '')
                return metric
        return None

    def get_flow_number(self, filename):
        """Extract flow number from filename"""
        parts = filename.split('-')
        for part in parts:
            if part.startswith('flow'):
                return int(part[4:])  # Remove 'flow' prefix and convert to int
        return None

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
                if flow_num is None:
                    print(f"Warning: Could not extract flow number from {file_path.name}")
                    continue
                # Skip if flow number is outside specified range
                if flow_num < FLOW_START or flow_num > FLOW_END:
                    continue
                files_by_flow.setdefault(flow_num, []).append(file_path)

        # Create plots for each flow
        for flow_num, files in sorted(files_by_flow.items()):
            flow_dir = protocol_plots_dir / f"flow{flow_num}"
            flow_dir.mkdir(exist_ok=True)

            for file_path in files:
                metric = self.get_metric_name(file_path.name)
                if metric is None:
                    print(f"Warning: Could not extract metric name from {file_path.name}")
                    continue
                    
                df = self.parse_data_file(file_path)

                if df is not None and not df.empty:
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

        # Find all possible metrics from all protocol directories
        all_metrics = set()
        for protocol in protocols:
            protocol_dir = self.data_dir / protocol
            for file_path in protocol_dir.glob("*.data"):
                if not file_path.name.endswith('.ascii'):
                    flow_num = self.get_flow_number(file_path.name)
                    if flow_num is not None and FLOW_START <= flow_num <= FLOW_END:
                        metric = self.get_metric_name(file_path.name)
                        if metric is not None:
                            all_metrics.add(metric)

        # Create combined plots for each flow and metric
        for flow_num in range(FLOW_START, FLOW_END + 1):
            flow_dir = combined_dir / f"flow{flow_num}"
            flow_dir.mkdir(exist_ok=True)

            for metric in all_metrics:
                valid_data = False
                plt.figure(figsize=(10, 6))
                
                for protocol in protocols:
                    file_path = self.data_dir / protocol / f"TcpVariantsComparison-flow{flow_num}-{metric}.data"
                    print(f"Checking file: {file_path}")
                    
                    if file_path.exists():
                        df = self.parse_data_file(file_path)
                        if df is not None and not df.empty:
                            plt.plot(df['time'], df['value'], '-o',
                                   color=self.colors[protocol],
                                   markersize=2,
                                   label=protocol)
                            valid_data = True
                            print(f"Plotted data for {protocol} {metric}")

                if valid_data:
                    plt.title(f'Flow {flow_num} - {metric} Comparison')
                    plt.xlabel('Time (s)')
                    plt.ylabel(metric.upper())
                    plt.grid(True)
                    plt.legend()
                    
                    plot_path = flow_dir / f'{metric}_comparison.png'
                    plt.savefig(plot_path)
                    print(f"Created combined plot: {plot_path}")
                else:
                    print(f"No valid data for flow {flow_num} metric {metric}")
                
                plt.close()

    def generate_all_plots(self):
        """Generate all individual and combined plots"""
        print(f"Generating plots for flows {FLOW_START} to {FLOW_END}")
        
        # Create individual protocol plots
        for protocol in ['TcpVeno', 'TcpWestwoodPlus']:
            print(f"\nProcessing {protocol}...")
            self.create_protocol_plots(protocol)
        
        # Create combined plots
        print("\nCreating combined plots...")
        self.create_combined_plots()

def main():
    # Example usage
    base_dir = "./results"  # Adjust this path as needed
    run_number = RUN_NUMBER  # Adjust run number as needed
    
    plotter = TCPProtocolPlotter(base_dir, run_number)
    plotter.generate_all_plots()

if __name__ == "__main__":
    main()