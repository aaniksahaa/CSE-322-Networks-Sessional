import xml.etree.ElementTree as ET
import os
import glob
import csv
import re

SIMULATION_TIME = 50 # in seconds

def parse_filename(filename):
    # Extract parameters from filename using regex
    pattern = r'aodv-analysis_nodeCount-(\d+)_packetsPerSecond-(\d+)_nodeSpeed-(\d+)\.flowmon'
    match = re.search(pattern, filename)
    if match:
        return {
            'nodeCount': int(match.group(1)),
            'packetsPerSecond': int(match.group(2)),
            'nodeSpeed': int(match.group(3))
        }
    return None

def analyze_flowmon(file_path):
    tree = ET.parse(file_path)
    root = tree.getroot()
    
    total_tx_bytes = total_rx_bytes = total_tx_packets = total_rx_packets = 0
    total_lost_packets = 0
    total_delay_sum = 0.0
    flow_count = 0

    for flow in root.findall(".//Flow"):
        flow_count += 1
        tx_bytes = int(flow.get("txBytes", "0"))
        rx_bytes = int(flow.get("rxBytes", "0"))
        tx_packets = int(flow.get("txPackets", "0"))
        rx_packets = int(flow.get("rxPackets", "0"))
        lost_packets = int(flow.get("lostPackets", "0"))
        delay_sum_ns = float(flow.get("delaySum", "0ns").rstrip("ns"))

        total_tx_bytes += tx_bytes
        total_rx_bytes += rx_bytes
        total_tx_packets += tx_packets
        total_rx_packets += rx_packets
        total_lost_packets += lost_packets
        total_delay_sum += delay_sum_ns

    # in MBps
    network_throughput = ((total_rx_bytes * 8)/(1024*1024)) / SIMULATION_TIME
    avg_end_to_end_delay = (total_delay_sum / total_rx_packets) / 1e9 if total_rx_packets > 0 else 0
    packet_drop_ratio = (total_lost_packets / total_tx_packets) if total_tx_packets > 0 else 0
    packet_delivery_ratio = (total_rx_packets / total_tx_packets) if total_tx_packets > 0 else 0

    return {
        'network_throughput': network_throughput,
        'avg_end_to_end_delay': avg_end_to_end_delay,
        'packet_drop_ratio': packet_drop_ratio,
        'packet_delivery_ratio': packet_delivery_ratio
    }

def main():
    TASK = 1
    # Find all flowmon files in the results/task-1 directory
    flowmon_files = glob.glob(f'./results/task-{TASK}/*.flowmon')
    
    # Prepare CSV output
    csv_headers = ['nodeCount', 'packetsPerSecond', 'nodeSpeed', 
                  'network_throughput', 'avg_end_to_end_delay', 
                  'packet_drop_ratio', 'packet_delivery_ratio']
    
    with open('flowmon_analysis.csv', 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=csv_headers)
        writer.writeheader()
        
        for file_path in flowmon_files:
            # Get parameters from filename
            params = parse_filename(os.path.basename(file_path))
            if not params:
                continue
                
            # Analyze the flowmon file
            results = analyze_flowmon(file_path)
            
            # Combine parameters and results
            row = {**params, **results}
            writer.writerow(row)

if __name__ == "__main__":
    main()