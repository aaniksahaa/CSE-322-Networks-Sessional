import os
from pathlib import Path

def format_path_for_latex(path):
    """Convert path to use forward slashes for LaTeX"""
    return str(path)[14:].replace(os.sep, '/')

def format_metric_name(metric):
    """Convert metric filename to readable title"""
    metric = metric.replace("_flow_averaged", "").replace("_all_flows_comparison", "")
    
    replacements = {
        "cwnd": "Congestion Window",
        "inflight": "In-Flight Packets",
        "next-rx": "Next RX",
        "next-tx": "Next TX",
        "rto": "Retransmission Timeout",
        "rtt": "Round Trip Time",
        "ssth": "Slow Start Threshold"
    }
    
    return replacements.get(metric, metric.upper())

def generate_latex_report():
    # Define paths
    base_path = Path("results/run-4/plots/combined")
    output_dir = Path("latex_report")
    output_dir.mkdir(exist_ok=True)

    # Define protocol pairs and their plot directories
    protocol_comparisons = [
        {
            "dir_name": "Veno-WestwoodPlus",
            "title": "TCP Veno vs TCP Westwood+",
            "plot_dir": "flowcombined",
            "description": "Comparison of the base TCP Veno and TCP Westwood+ variants"
        },
        {
            "dir_name": "WestwoodPlus-LogWestwoodPlus",
            "title": "TCP Westwood+ vs TCP Log Westwood+",
            "plot_dir": "flowaveraged",
            "description": "Analysis of the impact of logarithmic modifications to TCP Westwood+"
        },
        {
            "dir_name": "LogWestwoodPlus-StochasticLogWestwoodPlus",
            "title": "TCP Log Westwood+ vs TCP Stochastic Log Westwood+",
            "plot_dir": "flowaveraged",
            "description": "Evaluation of stochastic enhancements to Log TCP Westwood+"
        }
    ]

    # Start LaTeX content
    latex_content = [
        r"\documentclass[12pt,a4paper]{report}",
        r"\usepackage[utf8]{inputenc}",
        r"\usepackage[T1]{fontenc}",
        r"\usepackage{graphicx}",
        r"\usepackage[margin=1in]{geometry}",
        r"\usepackage{float}",
        r"\usepackage{titlesec}",
        r"\usepackage{caption}",
        r"\usepackage{fancyhdr}",
        r"\usepackage{hyperref}",
        r"\usepackage{booktabs}",
        r"\usepackage{xcolor}",
        r"\usepackage{amsmath}",
        r"\usepackage{amssymb}",
        r"\usepackage{amsfonts}",
        r"\usepackage{mathtools}",
        r"\usepackage{physics}",
        "",
        r"\titleformat{\chapter}[display]{\normalfont\huge\bfseries}{\chaptertitle}{20pt}{\Huge}",
        r"\pagestyle{fancy}",
        r"\fancyhf{}",
        r"\fancyhead[R]{\thepage}",
        r"\fancyhead[L]{\leftmark}",
        r"\renewcommand{\headrulewidth}{0.4pt}",
        "",
        r"\begin{document}",
        "",
        # Title page
        r"\begin{titlepage}",
        r"\begin{center}",
        r"{\Huge\bfseries Comparative Analysis of TCP Variants\\[1.5cm]}",
        r"{\Large\bfseries Performance Evaluation of Traditional and Enhanced TCP Implementations\\[1cm]}",
        r"\vfill",
        r"{\large Technical Report\\[0.5cm]}",
        r"{\large Network Performance Analysis Group\\[0.5cm]}",
        r"{\large \today}",
        r"\end{center}",
        r"\end{titlepage}",
        "",
        r"\tableofcontents",
        r"\newpage",
        "",
        # Introduction
        r"\chapter{Introduction}",
        r"\section{Background}",
        r"This report presents a detailed comparative analysis of various TCP variants, "
        r"focusing on traditional implementations and their enhanced versions. "
        r"The study examines the performance characteristics and behavioral patterns "
        r"of different TCP variants under controlled network conditions.\n",
        "",
        r"\chapter{Methodology}",
        r"\section{Experimental Setup}",
        r"The analysis was conducted using the ns-3 network simulator with the following configuration:",
        r"\begin{itemize}",
        r"\item Multiple concurrent TCP flows (0-6)",
        r"\item Controlled network conditions for consistent comparison",
        r"\item Comprehensive metric collection including cwnd, RTT, RTO",
        r"\item Data averaging across flows for statistical significance",
        r"\end{itemize}",
        "",
        r"\section{Metrics Analyzed}",
        r"\begin{itemize}",
        r"\item Congestion Window (cwnd): Measures the sending rate adaptation",
        r"\item Round Trip Time (RTT): Indicates network latency",
        r"\item Retransmission Timeout (RTO): Shows congestion response",
        r"\item In-Flight Packets: Represents active network utilization",
        r"\item Slow Start Threshold (ssth): Indicates congestion control transitions",
        r"\end{itemize}",
        r"\newpage"
        r"\section{Proposed Variant of Westwood+}",
        r"\subsection{Intuition}",
        "The traditional TCP Westwood+ algorithm uses a fixed value of $\\alpha$ for all flows in the network. " +
        "However, this leads to similar congestion window growth patterns across different flows, potentially causing " +
        "synchronized packet losses. Our proposed variant introduces stochastic behavior by varying $\\alpha$ according " +
        "to a normal distribution, leading to more diverse congestion window growth patterns across flows.",
        "",
        r"\subsection{Mathematical Proof}",
        "From the plot it is evident that, the time after which the incrementing portion reaches a steady state is important, " +
        "since near that point, the cwnd tends to drop due to packet loss. " +
        "One issue with the traditional algorithms is that, this time is roughly being the same for all the flows in the network since, " +
        "we are using the same value of $\\alpha$ for every node.",
        "",
        "We now proceed to express this time $T$, taken to reach an approximately steady state from the initial state in terms of $\\alpha$. " +
        "Then we will use that relation to derive the distribution of the chosen value of $\\alpha$, which leads to our desired distribution of the time $T$.",
        r"\begin{equation*}",
        r"W \leftarrow W + \frac{W_{max} - W}{\alpha W}",
        r"\end{equation*}",
        "",
        "To model this simply, let us assume $W_{max} = k$ and that we are starting at $t = 0$ with $W = \\frac{k}{3}$",
        r"\begin{equation*}",
        r"\text{Now,} \quad \frac{dW}{dt} = \frac{k-W}{\alpha W}",
        r"\end{equation*}",
        r"\begin{equation*}",
        r"\Rightarrow \frac{W dW}{k-W} = \frac{dt}{\alpha}",
        r"\end{equation*}",
        r"\begin{equation*}",
        r"\Rightarrow \int \frac{W dW}{k-W} = \int \frac{dt}{\alpha}",
        r"\end{equation*}",
        "",
        "Now, assume we want to find the time to reach from $\\frac{k}{3}$ to $\\frac{9k}{10}$ [since reaching exactly $k$ is not possible in finite time, " +
        "in fact packet loss will occur before that]",
        r"\begin{equation*}",
        r"\text{So,} \quad \int_{\frac{k}{3}}^{\frac{9k}{10}} \frac{W dW}{k-W} = \int_{0}^{T} \frac{dt}{\alpha}",
        r"\end{equation*}",
        r"\begin{equation*}",
        r"\Rightarrow -\int_{\frac{k}{3}}^{\frac{9k}{10}} \left(1 - \frac{k}{k-W}\right) dW = \frac{T}{\alpha}",
        r"\end{equation*}",
        r"\begin{equation*}",
        r"\Rightarrow \left[W + k\ln|k-W|\right]_{\frac{k}{3}}^{\frac{9k}{10}} = -\frac{T}{\alpha}",
        r"\end{equation*}",
        r"\begin{equation*}",
        r"\Rightarrow T = -\alpha\left[\frac{17}{30}k - 1.897k\right]",
        r"\end{equation*}",
        r"\begin{equation*}",
        r"\Rightarrow T = 1.3305\alpha k",
        r"\end{equation*}",
        r"\begin{equation*}",
        r"\Rightarrow T = 1.3305\alpha W_{max}",
        r"\end{equation*}",
        "",
        "Therefore, regardless of our assumptions about the initial and stopping conditions, T is always directly proportional to our choice of $\\alpha$",
        r"\begin{equation*}",
        r"T \propto \alpha",
        r"\end{equation*}",
        "",
        "Thus, to enforce that time $T$ varies according to a normal distribution, we also need to enforce that $\\alpha$ follows a normal distribution across different flows.",
        r"\newpage"
    ]

    # Results chapter
    latex_content.append(r"\chapter{Results and Analysis}")

    # Process each protocol comparison
    for comparison in protocol_comparisons:
        latex_content.extend([
            f"\\section{{{comparison['title']}}}",
            f"\\noindent {comparison['description']}.\n",
            ""
        ])

        # Get plot directory
        plot_dir = base_path / comparison['dir_name'] / comparison['plot_dir']
        
        # Define metrics to look for (both averaged and comparison versions)
        base_metrics = [
            "cwnd", "inflight", "next-rx", "next-tx", "rto", "rtt", "ssth"
        ]
        
        # Look for either flow_averaged or all_flows_comparison versions
        for base_metric in base_metrics:
            averaged_name = f"{base_metric}_flow_averaged"
            comparison_name = f"{base_metric}_all_flows_comparison"
            
            # Check both possible filenames
            plot_path = None
            if (plot_dir / f"{averaged_name}.png").exists():
                plot_path = plot_dir / f"{averaged_name}.png"
            elif (plot_dir / f"{comparison_name}.png").exists():
                plot_path = plot_dir / f"{comparison_name}.png"
                
            if plot_path:
                metric_title = format_metric_name(base_metric)
                latex_plot_path = format_path_for_latex(plot_path)
                latex_content.extend([
                    r"\newpage",
                    r"\subsection{" + metric_title + "}",
                    r"\begin{figure}[H]",
                    r"\centering",
                    f"\\includegraphics[width=0.95\\textwidth]{{{latex_plot_path}}}",
                    f"\\caption{{{metric_title} Analysis for {comparison['title']}}}",
                    r"\label{fig:" + f"{comparison['dir_name']}_{base_metric}" + "}",
                    r"\end{figure}",
                    "",
                    # r"\noindent\textbf{Analysis:}\\",
                    # f"[Detailed analysis of {metric_title} comparison between {comparison['title']} goes here. ",
                    # "Include observations about:", 
                    # r"\begin{itemize}",
                    # r"\item Relative performance differences",
                    # r"\item Notable patterns or behaviors",
                    # r"\item Implications of the results",
                    # r"\item Any anomalies or unexpected findings",
                    # r"\end{itemize}]",
                    r"\newpage",
                    ""
                ])

                # For ssth log plots
                if base_metric == "ssth":
                    log_plot = plot_path.parent / f"{plot_path.stem}_log.png"
                    if log_plot.exists():
                        latex_log_path = format_path_for_latex(log_plot)
                        latex_content.extend([
                            r"\subsection{" + metric_title + " (Logarithmic Scale)}",
                            r"\begin{figure}[H]",
                            r"\centering",
                            f"\\includegraphics[width=0.95\\textwidth]{{{latex_log_path}}}",
                            f"\\caption{{{metric_title} Analysis (Log Scale) for {comparison['title']}}}",
                            r"\label{fig:" + f"{comparison['dir_name']}_{base_metric}_log" + "}",
                            r"\end{figure}",
                            "",
                            # r"\noindent\textbf{Analysis:}\\",
                            # "[Analysis of logarithmic scale visualization...]",
                            r"\newpage",
                            ""
                        ])

    # Conclusions
    latex_content.extend([
        r"\chapter{Conclusions}",
        r"\section{Key Findings}",
        r"\begin{itemize}",
        r"\item Comparative performance of TCP variants",
        r"\item Impact of logarithmic modifications",
        r"\item Effects of stochastic enhancements",
        r"\end{itemize}",
        "",
        r"\end{document}"
    ])

    # Write to file
    with open(output_dir / "report.tex", "w") as f:
        f.write("\n".join(latex_content))

if __name__ == "__main__":
    generate_latex_report()