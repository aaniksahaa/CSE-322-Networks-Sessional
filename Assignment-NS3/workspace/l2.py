import pandas as pd
import os

def generate_latex_report(csv_path, plots_dir, output_file):
    # Read CSV
    df = pd.read_csv(csv_path)
    
    # Find column-wise maximums
    max_indices = {}
    for col in df.columns[3:]:  # Skip first 3 columns (input params)
        max_indices[col] = df[col].idxmax()
    
    # Start LaTeX content
    latex_content = r'''
\documentclass{article}
\usepackage{graphicx}
\usepackage{booktabs}
\usepackage{float}
\usepackage{siunitx}
\usepackage{geometry}
\geometry{margin=2.5cm}

\title{Network Simulation Analysis Report}
\author{Generated Report}
\date{\today}

\begin{document}
\maketitle

\section{Introduction}
This report presents the analysis of network simulation results using various parameters including node count, packets per second, and node speed.

\section{Parameter Analysis}
\subsection{Single Parameter Impact}
The following figures show how individual parameters affect network metrics while keeping other parameters constant.

\begin{figure}[h]
\centering
\includegraphics[width=0.8\textwidth]{plots/task-1/single-input-params/analysis_nodeCount}
\caption{Impact of Node Count on Network Metrics}
\end{figure}

\begin{figure}[h]
\centering
\includegraphics[width=0.8\textwidth]{plots/task-1/single-input-params/analysis_packetsPerSecond}
\caption{Impact of Packets Per Second on Network Metrics}
\end{figure}

\begin{figure}[h]
\centering
\includegraphics[width=0.8\textwidth]{plots/task-1/single-input-params/analysis_nodeSpeed}
\caption{Impact of Node Speed on Network Metrics}
\end{figure}

\subsection{Parameter Interactions}
Heat maps showing the interaction between pairs of parameters:

\begin{figure}[h]
\centering
\includegraphics[width=0.8\textwidth]{plots/task-1/two-params-heatmaps/heatmap_nodeCount_packetsPerSecond}
\caption{Heatmap: Node Count vs Packets Per Second}
\end{figure}

\begin{figure}[h]
\centering
\includegraphics[width=0.8\textwidth]{plots/task-1/two-params-heatmaps/heatmap_nodeCount_nodeSpeed}
\caption{Heatmap: Node Count vs Node Speed}
\end{figure}

\begin{figure}[h]
\centering
\includegraphics[width=0.8\textwidth]{plots/task-1/two-params-heatmaps/heatmap_packetsPerSecond_nodeSpeed}
\caption{Heatmap: Packets Per Second vs Node Speed}
\end{figure}

\section{Detailed Results}
The following table shows the complete simulation results. Bold values indicate maximum values for each metric.
'''

    # Generate table
    latex_content += r'\begin{table}[h]' + '\n'
    latex_content += r'\centering\small' + '\n'
    latex_content += r'\caption{Complete Simulation Results}' + '\n'
    latex_content += r'\begin{tabular}{' + ' '.join(['l'] * len(df.columns)) + '}' + '\n'
    latex_content += r'\toprule' + '\n'
    
    # Headers
    headers = ['Node Count', 'PPS', 'Speed', 'Throughput', 'End-to-End Delay', 'Drop Ratio', 'Delivery Ratio']
    latex_content += ' & '.join(headers) + r' \\' + '\n'
    latex_content += r'\midrule' + '\n'
    
    # Data rows
    for _, row in df.iterrows():
        row_str = []
        for i, col in enumerate(df.columns):
            val = f"{row[col]:.2f}" if i >= 3 else f"{int(row[col])}"
            if i >= 3 and row.name == max_indices[col]:
                val = r'\textbf{' + val + '}'
            row_str.append(val)
        latex_content += ' & '.join(row_str) + r' \\' + '\n'
    
    latex_content += r'\bottomrule' + '\n'
    latex_content += r'\end{tabular}' + '\n'
    latex_content += r'\end{table}' + '\n'
    
    # Close document
    latex_content += r'\end{document}' + '\n'
    
    # Write to file
    with open(output_file, 'w') as f:
        f.write(latex_content)

if __name__ == "__main__":
    generate_latex_report(
        'flowmon_analysis.csv',
        'plots/task-1',
        'network_analysis_report.tex'
    )
