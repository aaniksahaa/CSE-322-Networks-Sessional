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
    latex_content = r'''\documentclass{article}
\usepackage{graphicx}
\usepackage{booktabs}
\usepackage{float}
\usepackage{subfig}
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
\begin{figure}[H]
\centering
\subfloat[Node Count Impact]{\includegraphics[width=0.3\textwidth]{plots/task-1/single-input-params/analysis_nodeCount}}
\subfloat[Packets Per Second Impact]{\includegraphics[width=0.3\textwidth]{plots/task-1/single-input-params/analysis_packetsPerSecond}}
\subfloat[Node Speed Impact]{\includegraphics[width=0.3\textwidth]{plots/task-1/single-input-params/analysis_nodeSpeed}}
\caption{Impact of Individual Parameters on Network Metrics}
\end{figure}

\subsection{Parameter Interactions}
Heat maps showing the interaction between pairs of parameters:
\begin{figure}[H]
\centering
\subfloat[Node Count vs PPS]{\includegraphics[width=0.3\textwidth]{plots/task-1/two-params-heatmaps/heatmap_nodeCount_packetsPerSecond}}
\subfloat[Node Count vs Speed]{\includegraphics[width=0.3\textwidth]{plots/task-1/two-params-heatmaps/heatmap_nodeCount_nodeSpeed}}
\subfloat[PPS vs Speed]{\includegraphics[width=0.3\textwidth]{plots/task-1/two-params-heatmaps/heatmap_packetsPerSecond_nodeSpeed}}
\caption{Parameter Interaction Analysis}
\end{figure}

\section{Detailed Results}
The following table shows the complete simulation results. Bold values indicate maximum values for each metric.
'''

    # Generate table
    latex_content += r'\begin{table}[H]'
    latex_content += r'\centering\small'
    latex_content += r'\caption{Complete Simulation Results}'
    latex_content += r'\begin{tabular}{SSSSSSSS}'
    latex_content += r'\toprule'
    
    # Headers
    headers = ['Nodes', 'PPS', 'Speed', 'Throughput', 'Delay', 'Drop Ratio', 'Delivery Ratio']
    latex_content += ' & '.join(headers) + r'\\'
    latex_content += r'\midrule'
    
    # Data rows
    for _, row in df.iterrows():
        row_str = []
        for i, col in enumerate(df.columns):
            val = f"{row[col]:.2f}" if i >= 3 else f"{int(row[col])}"
            if i >= 3 and row.name == max_indices[col]:
                val = r'\textbf{' + val + '}'
            row_str.append(val)
        latex_content += ' & '.join(row_str) + r'\\'
    
    latex_content += r'\bottomrule'
    latex_content += r'\end{tabular}'
    latex_content += r'\end{table}'
    
    # Close document
    latex_content += r'\end{document}'
    
    # Write to file
    with open(output_file, 'w') as f:
        f.write(latex_content)

if __name__ == "__main__":
    generate_latex_report(
        'flowmon_analysis.csv',
        'plots/task-1',
        'network_analysis_report.tex'
    )