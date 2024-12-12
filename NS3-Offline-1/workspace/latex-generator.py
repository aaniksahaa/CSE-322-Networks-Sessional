import os

def generate_latex():
    latex_content = [
        # Preamble
        r"\documentclass{article}",
        r"\usepackage{graphicx}",
        r"\usepackage{float}",
        r"\usepackage[margin=1in]{geometry}",
        r"\begin{document}",
        
        # Title
        r"\title{Performance Analysis of AODV, RAODV and modified RAODV Routing Protocols}",
        r"\author{Anik Saha, ID: 2005001}",
        r"\maketitle",
        r"\newpage",
        
        # Introduction
        r"\section{Introduction}",
        r"This report presents a comprehensive analysis of the performance metrics for AODV (Ad hoc On-demand Distance Vector), RAODV (Reliable AODV) and two other modifications of RAODV. The analysis includes comparisons of various performance parameters across different network conditions.",
        
        r"\subsection*{Input Parameters:}",
        r"\begin{itemize}",
        r"    \item Node Count",
        r"    \item Packets per second", 
        r"    \item Node Speed",
        r"\end{itemize}",
        
        r"\subsection*{Output Parameters:}",
        r"\begin{itemize}",
        r"    \item Network Throughput",
        r"    \item End-to-end Delay",
        r"    \item Packet Drop Ratio (\%)",
        r"    \item Packet Delivery Ratio (\%)",
        r"\end{itemize}",
        
        r"\subsection*{Routing Protocols:}",
        r"\begin{itemize}",
        r"    \item \textbf{AODV:} In case of Ad-hoc On-demand Distance Vector routing protocol, we broadcast a RREQ packet from the sender end and upon receiving it as the destination or an intermediate which knows the path to destination, we unicast a RREP message.",
        r"    ",
        r"    \item \textbf{RAODV:} The core point of change in case of RAODV is that, here we also broadcast the reverse message rather than unicasting and we name it as REV\_RREQ message.",
        r"    ",
        r"    \item \textbf{RAODV-v1(Variant 1):} I propose this following modification of RAODV protocol where I also unicast the RREP message rather than just broadcasting the REV\_RREQ message.",
        r"",
        r"    \textbf{Intuition:} The unicast path we found by broadcasting is surely more promising in terms of link-breakage resistance since it was the shortest path found. Now, rather than entirely ignoring the unicast path, it seems better to keep both the unicast and broadcast approaches.",
        r"    ",
        r"    \item \textbf{RAODV-v2(Variant 2):} The second modification of RAODV protocol I propose is sending RREP packets in multiple unicast paths rather than broadcasting. From an implementation perspective, this reduces to removing the idCache checking when the RREQ packet reaches the actual destination.",
        r"",
        r"    \textbf{Intuition:} In this modification, I entirely remove the broadcast and replace it with multiple unicast messages. In this way, rather than replying on one shortest path found in the RREQ broadcasting approach of original AODV, we take into account all the different unicast paths through which RREQ packet reached the destination.",
        r"\end{itemize}",
        
        # AODV Section
        r"\section{AODV Performance Analysis}",
        
        r"\subsection{Node Count Analysis}",
        r"\begin{figure}[H]",
        r"\centering",
        r"\includegraphics[width=0.8\textwidth]{plots/aodv/performance_vs_nodeCount_aodv.png}",
        r"\caption{AODV Performance vs Node Count}",
        r"\end{figure}",
        r"This figure demonstrates how AODV performance varies with different node counts in the network.",
        r"\newpage",
        
        r"\subsection{Node Speed Analysis}",
        r"\begin{figure}[H]",
        r"\centering",
        r"\includegraphics[width=0.8\textwidth]{plots/aodv/performance_vs_nodeSpeed_aodv.png}",
        r"\caption{AODV Performance vs Node Speed}",
        r"\end{figure}",
        r"This analysis shows the impact of node mobility speed on AODV protocol performance.",
        r"\newpage",
        
        r"\subsection{Packets Per Second Analysis}",
        r"\begin{figure}[H]",
        r"\centering",
        r"\includegraphics[width=0.8\textwidth]{plots/aodv/performance_vs_packetsPerSecond_aodv.png}",
        r"\caption{AODV Performance vs Packets Per Second}",
        r"\end{figure}",
        r"This figure shows how packet transmission rate affects AODV performance.",
        r"\newpage",
        
        # RAODV Section
        r"\section{RAODV Performance Analysis}",
        
        r"\subsection{Node Count Analysis}",
        r"\begin{figure}[H]",
        r"\centering",
        r"\includegraphics[width=0.8\textwidth]{plots/raodv/performance_vs_nodeCount_raodv.png}",
        r"\caption{RAODV Performance vs Node Count}",
        r"\end{figure}",
        r"This figure demonstrates how RAODV performance varies with different node counts.",
        r"\newpage",
        
        r"\subsection{Node Speed Analysis}",
        r"\begin{figure}[H]",
        r"\centering",
        r"\includegraphics[width=0.8\textwidth]{plots/raodv/performance_vs_nodeSpeed_raodv.png}",
        r"\caption{RAODV Performance vs Node Speed}",
        r"\end{figure}",
        r"This analysis shows the impact of node mobility speed on RAODV protocol performance.",
        r"\newpage",
        
        r"\subsection{Packets Per Second Analysis}",
        r"\begin{figure}[H]",
        r"\centering",
        r"\includegraphics[width=0.8\textwidth]{plots/raodv/performance_vs_packetsPerSecond_raodv.png}",
        r"\caption{RAODV Performance vs Packets Per Second}",
        r"\end{figure}",
        r"This figure shows how packet transmission rate affects RAODV performance.",
        r"\newpage",
        
        # Comparison Section
        r"\section{AODV vs RAODV Comparison}",
        
        r"\subsection{Node Count Comparison}",
        r"\begin{figure}[H]",
        r"\centering",
        r"\includegraphics[width=0.8\textwidth]{plots/two-comp/performance_vs_nodeCount_aodv_raodv.png}",
        r"\caption{Node Count Performance Comparison}",
        r"\end{figure}",
        r"This comparison demonstrates the relative performance of AODV and RAODV across different node counts.",
        r"\newpage",
        
        r"\subsection{Node Speed Comparison}",
        r"\begin{figure}[H]",
        r"\centering",
        r"\includegraphics[width=0.8\textwidth]{plots/two-comp/performance_vs_nodeSpeed_aodv_raodv.png}",
        r"\caption{Node Speed Performance Comparison}",
        r"\end{figure}",
        r"This analysis compares how node mobility speed affects both protocols.",
        r"\newpage",
        
        r"\subsection{Packets Per Second Comparison}",
        r"\begin{figure}[H]",
        r"\centering",
        r"\includegraphics[width=0.8\textwidth]{plots/two-comp/performance_vs_packetsPerSecond_aodv_raodv.png}",
        r"\caption{Packets Per Second Performance Comparison}",
        r"\end{figure}",
        r"This figure compares how packet transmission rate affects both protocols.",
        r"\newpage",
        
        # Multi-variant Comparison Section
        r"\section{Multi-variant RAODV Comparison}",
        
        r"\subsection{Node Count Multi-comparison}",
        r"\begin{figure}[H]",
        r"\centering",
        r"\includegraphics[width=0.8\textwidth]{plots/multi-comp/performance_vs_nodeCount_aodv_raodv_raodv-v1_raodv-v2.png}",
        r"\caption{Node Count Performance Multi-comparison}",
        r"\end{figure}",
        r"This comparison shows performance across different node counts for all protocol variants.",
        r"\newpage",
        
        r"\subsection{Node Speed Multi-comparison}",
        r"\begin{figure}[H]",
        r"\centering",
        r"\includegraphics[width=0.8\textwidth]{plots/multi-comp/performance_vs_nodeSpeed_aodv_raodv_raodv-v1_raodv-v2.png}",
        r"\caption{Node Speed Performance Multi-comparison}",
        r"\end{figure}",
        r"This analysis compares how node mobility speed affects all protocol variants.",
        r"\newpage",
        
        r"\subsection{Packets Per Second Multi-comparison}",
        r"\begin{figure}[H]",
        r"\centering",
        r"\includegraphics[width=0.8\textwidth]{plots/multi-comp/performance_vs_packetsPerSecond_aodv_raodv_raodv-v1_raodv-v2.png}",
        r"\caption{Packets Per Second Performance Multi-comparison}",
        r"\end{figure}",
        r"This figure compares how packet transmission rate affects all protocol variants.",
        
        # End document
        r"\end{document}"
    ]
    
    # Write to file
    with open('report.tex', 'w') as f:
        f.write('\n'.join(latex_content))

if __name__ == "__main__":
    generate_latex()
    print("LaTeX file 'report.tex' has been generated successfully.")