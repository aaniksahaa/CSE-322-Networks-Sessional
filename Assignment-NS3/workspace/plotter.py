import pandas as pd
import matplotlib.pyplot as plt
import os
import seaborn as sns

from util import *

TAKE_OTHERS_INDEX = 0

def create_analysis_plots(csv_path, output_dir):
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    # Read the CSV file
    df = pd.read_csv(csv_path)
    
    # Get unique values for each parameter
    params = {
        'nodeCount': sorted(df['nodeCount'].unique()),
        'packetsPerSecond': sorted(df['packetsPerSecond'].unique()),
        'nodeSpeed': sorted(df['nodeSpeed'].unique())
    }
    
    # Get the second minimum value for each parameter
    base_values = {
        'nodeCount': params['nodeCount'][TAKE_OTHERS_INDEX],
        'packetsPerSecond': params['packetsPerSecond'][TAKE_OTHERS_INDEX],
        'nodeSpeed': params['nodeSpeed'][TAKE_OTHERS_INDEX]
    }
    
    # Output metrics to plot
    metrics = ['network_throughput', 'avg_end_to_end_delay', 
              'packet_drop_ratio', 'packet_delivery_ratio']
    
    # Define a custom color palette
    palette = sns.color_palette("husl", len(metrics))
    
    # Plot for each varying parameter
    for varying_param in ['nodeCount', 'packetsPerSecond', 'nodeSpeed']:
        # Create a figure with subplots for each metric
        fig, axes = plt.subplots(2, 2, figsize=(15, 12))
        fig.suptitle(f'Impact of {varying_param} on Network Metrics', fontsize=16)
        
        # Filter data where other parameters are at their base values
        filter_conditions = {
            param: base_values[param] 
            for param in base_values 
            if param != varying_param
        }
        
        filtered_df = df.copy()
        for param, value in filter_conditions.items():
            filtered_df = filtered_df[filtered_df[param] == value]
            
        # Plot each metric
        for idx, metric in enumerate(metrics):
            ax = axes[idx // 2, idx % 2]
            
            sns.lineplot(data=filtered_df, x=varying_param, y=metric, 
                        marker='o', ax=ax, color=palette[idx])
            
            ax.set_title(f'{metric.replace("_", " ").title()}', color=palette[idx])
            ax.grid(True)
            ax.set_xlabel(varying_param)
            ax.set_ylabel(metric.replace("_", " ").title())
            
        plt.tight_layout(rect=[0, 0, 1, 0.95])  # Adjust layout to fit title
        plt.savefig(os.path.join(output_dir, f'analysis_{varying_param}.png'))
        plt.close()


def create_heatmap_plots(csv_path, output_dir):
    os.makedirs(output_dir, exist_ok=True)
    df = pd.read_csv(csv_path)
    
    param_pairs = [
        ('nodeCount', 'packetsPerSecond'),
        ('nodeCount', 'nodeSpeed'),
        ('packetsPerSecond', 'nodeSpeed')
    ]
    
    metrics = ['network_throughput', 'avg_end_to_end_delay', 
              'packet_drop_ratio', 'packet_delivery_ratio']
    
    for param1, param2 in param_pairs:
        fig, axes = plt.subplots(2, 2, figsize=(15, 12))
        fig.suptitle(f'Heatmaps for {param1} vs {param2}', fontsize=16)
        
        for idx, metric in enumerate(metrics):
            ax = axes[idx // 2, idx % 2]
            
            # Create pivot table for heatmap
            pivot_data = df.pivot_table(
                values=metric,
                index=param1,
                columns=param2,
                aggfunc='mean'
            )
            
            # Create heatmap
            sns.heatmap(pivot_data, ax=ax, cmap='viridis', 
                       annot=True, fmt='.2f', cbar=True)
            
            ax.set_title(metric.replace('_', ' ').title())
        
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f'heatmap_{param1}_{param2}.png'))
        plt.close()

if __name__ == "__main__":
    csv_filepath = f'{RESULT_SUMMARY_DIR}/flowmon_analysis.csv'
    create_analysis_plots(csv_filepath, f'{PLOTS_DIR}/single-input-params')
    create_heatmap_plots(csv_filepath, f'{PLOTS_DIR}/two-params-heatmaps')