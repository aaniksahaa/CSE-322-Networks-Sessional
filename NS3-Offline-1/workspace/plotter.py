import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from util import *

IMAGE_DPI = 100

default_values = {
    'nodeCount': 20,
    'packetsPerSecond': 100,
    'nodeSpeed': "5 m/s",
}

def create_performance_plots(csv_file, output_dir, protocolNames):
    df = pd.read_csv(csv_file)
    df = df[df['protocolName'].isin(protocolNames)]
    input_params = df['comment'].unique()
    output_params = ['network_throughput', 'avg_end_to_end_delay', 'packet_drop_ratio', 'packet_delivery_ratio']
    output_labels = {
        'network_throughput': 'Network Throughput (MBps)',
        'avg_end_to_end_delay': 'Average End-to-End Delay (s)',
        'packet_drop_ratio': 'Packet Drop Ratio (%)',
        'packet_delivery_ratio': 'Packet Delivery Ratio (%)'
    }

    for param in input_params:
        default_suffixes = []
        for p in input_params:
            if(p != param):
                default_suffixes.append(f"{p} = {default_values[p]}")
        suff = ", ".join(default_suffixes)

        fig, axes = plt.subplots(2, 2, figsize=(15, 12))
        fig.suptitle(f'Network Performance Metrics vs {param} ({suff}) - {", ".join(protocolNames)}', fontsize=16, y=1.02)
        axes = axes.flatten()

        for idx, output_param in enumerate(output_params):
            adf = pd.read_csv(csv_file)
            
            param_data = adf[adf['comment'] == param]

            ax = axes[idx]
            
            for protocol in protocolNames:
                protocol_data = param_data[param_data['protocolName'] == protocol]
                sns.lineplot(
                    data=protocol_data,
                    x=param,
                    y=output_param,
                    marker='o',
                    linewidth=2,
                    markersize=8,
                    label=protocol,
                    ax=ax
                )
            
            ax.set_xlabel(f'{param}', fontsize=12)
            ax.set_ylabel(output_labels[output_param], fontsize=12)
            ax.grid(True, linestyle='--', alpha=0.7)
            ax.legend(title='Protocol')

            # Add value labels
            for protocol in protocolNames:
                protocol_data = param_data[param_data['protocolName'] == protocol]
                for x, y in zip(protocol_data[param], protocol_data[output_param]):
                    ax.annotate(f'{y:.2f}', 
                               (x, y),
                               xytext=(5, 5),
                               textcoords='offset points',
                               fontsize=8)

        plt.tight_layout()
        plt.savefig(f'{output_dir}/performance_vs_{param}_{"_".join(protocolNames).lower()}.png', dpi=IMAGE_DPI, bbox_inches='tight')
        plt.close()

if __name__ == "__main__":
    csv_filepath = f'{RESULT_SUMMARY_DIR}/flowmon_analysis.csv'
    
    output_dir = f'{PLOTS_DIR}/aodv'
    protocolNames = ["AODV"]
    create_performance_plots(csv_filepath, output_dir, protocolNames)

    output_dir = f'{PLOTS_DIR}/raodv'
    protocolNames = ["RAODV"]
    create_performance_plots(csv_filepath, output_dir, protocolNames)

    output_dir = f'{PLOTS_DIR}/two-comp'
    protocolNames = ["AODV", "RAODV"]
    create_performance_plots(csv_filepath, output_dir, protocolNames)

    output_dir = f'{PLOTS_DIR}/multi-comp'
    protocolNames = ["AODV", "RAODV", "RAODV-v1", "RAODV-v2"]
    create_performance_plots(csv_filepath, output_dir, protocolNames)