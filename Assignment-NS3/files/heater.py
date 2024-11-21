import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

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
    create_heatmap_plots('flowmon_analysis.csv', './plots/two-params-heatmaps')