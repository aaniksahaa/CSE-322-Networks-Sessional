import pandas as pd
import matplotlib.pyplot as plt
import os
import seaborn as sns

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
        'nodeCount': params['nodeCount'][1],
        'packetsPerSecond': params['packetsPerSecond'][1],
        'nodeSpeed': params['nodeSpeed'][1]
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

if __name__ == "__main__":
    create_analysis_plots('flowmon_analysis.csv', './plots/single-input-params')
