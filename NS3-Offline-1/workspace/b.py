import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

from util import *

def create_performance_plots(csv_file, output_dir):
    # Read the CSV file
    df = pd.read_csv(csv_file)
    
    # Get unique comments (input parameters to analyze)
    input_params = df['comment'].unique()
    
    # Output parameters to plot
    output_params = [
        'network_throughput',
        'avg_end_to_end_delay',
        'packet_drop_ratio',
        'packet_delivery_ratio'
    ]
    
    # Pretty labels for output parameters
    output_labels = {
        'network_throughput': 'Network Throughput (MBps)',
        'avg_end_to_end_delay': 'Average End-to-End Delay (s)',
        'packet_drop_ratio': 'Packet Drop Ratio (%)',
        'packet_delivery_ratio': 'Packet Delivery Ratio (%)'
    }
    
    # Create a plot for each input parameter
    for param in input_params:
        # Filter data for current parameter
        param_data = df[df['comment'] == param]
        
        # Create figure with 4 subplots
        fig, axes = plt.subplots(2, 2, figsize=(15, 12))
        fig.suptitle(f'Network Performance Metrics vs {param}', fontsize=16, y=1.02)
        
        # Flatten axes array for easier iteration
        axes = axes.flatten()
        
        # Create a subplot for each output parameter
        for idx, output_param in enumerate(output_params):
            ax = axes[idx]
            
            # Create line plot
            sns.lineplot(
                data=param_data,
                x=param,
                y=output_param,
                marker='o',
                linewidth=2,
                markersize=8,
                ax=ax
            )
            
            # Customize subplot
            ax.set_xlabel(f'{param}', fontsize=12)
            ax.set_ylabel(output_labels[output_param], fontsize=12)
            ax.grid(True, linestyle='--', alpha=0.7)
            
            # Add value labels on points
            for x, y in zip(param_data[param], param_data[output_param]):
                ax.annotate(f'{y:.2f}', 
                           (x, y),
                           xytext=(5, 5),
                           textcoords='offset points',
                           fontsize=10)
        
        # Adjust layout and save plot
        plt.tight_layout()
        plt.savefig(f'{output_dir}/performance_vs_{param}.png', dpi=300, bbox_inches='tight')
        plt.close()

if __name__ == "__main__":
    csv_filepath = f'{RESULT_SUMMARY_DIR}/flowmon_analysis.csv'
    output_dir = f'{PLOTS_DIR}'
    create_performance_plots(csv_filepath, output_dir)