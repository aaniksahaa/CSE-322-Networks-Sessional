import xml.etree.ElementTree as ET
import os
import glob
import csv
import re
from util import *

SIMULATION_TIME = 0 # not initialized, will be read from the filenames

def parse_filename(filename):
    # Extract parameters from filename using regex
    pattern = r'experiment_protocolName-(\w+)_nodeCount-(\d+)_packetsPerSecond-(\d+)_nodeSpeed-(\d+)_flowRunningTime-(\d+)_comment-(\w+)\.'
    match = re.search(pattern, filename)
    global SIMULATION_TIME
    SIMULATION_TIME = int(match.group(5))
    if match:
        return {
            'protocolName': match.group(1),
            'nodeCount': int(match.group(2)),
            'packetsPerSecond': int(match.group(3)),
            'nodeSpeed': int(match.group(4)),
            'flowRunningTime': int(match.group(5)),
            'comment': match.group(6)
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

    # Calculate metrics
    simulation_time = SIMULATION_TIME

    # in MBps
    network_throughput = ((total_rx_bytes * 8)/(1024*1024)) / simulation_time
    avg_end_to_end_delay = (total_delay_sum / total_rx_packets) / 1e9 if total_rx_packets > 0 else 0
    packet_drop_ratio = 100*(total_lost_packets / total_tx_packets) if total_tx_packets > 0 else 0
    packet_delivery_ratio = 100*(total_rx_packets / total_tx_packets) if total_tx_packets > 0 else 0

    return {
        'network_throughput': network_throughput,
        'avg_end_to_end_delay': avg_end_to_end_delay,
        'packet_drop_ratio': packet_drop_ratio,
        'packet_delivery_ratio': packet_delivery_ratio,
        'total_flows': flow_count,
        'total_tx_bytes': total_tx_bytes,
        'total_rx_bytes': total_rx_bytes,
        'total_tx_packets': total_tx_packets,
        'total_rx_packets': total_rx_packets,
        'total_lost_packets': total_lost_packets
    }

def main():
    # Find all flowmon files
    flowmon_files = glob.glob(f'{RESULT_OUTPUTS_DIR}/*.flowmon')
    
    # Prepare CSV output with all parameters
    csv_headers = [
        'protocolName', 'nodeCount', 'packetsPerSecond', 'nodeSpeed', 
        'flowRunningTime', 'comment', 'network_throughput', 
        'avg_end_to_end_delay', 'packet_drop_ratio', 
        'packet_delivery_ratio', 'total_flows', 'total_tx_bytes',
        'total_rx_bytes', 'total_tx_packets', 'total_rx_packets',
        'total_lost_packets'
    ]
    
    with open(f'{RESULT_SUMMARY_DIR}/flowmon_analysis.csv', 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=csv_headers)
        writer.writeheader()
        
        for file_path in flowmon_files:
            # Get parameters from filename
            params = parse_filename(os.path.basename(file_path))
            if not params:
                print(f"Warning: Could not parse parameters from filename: {file_path}")
                continue
                
            # Analyze the flowmon file
            try:
                results = analyze_flowmon(file_path)
                
                # Combine parameters and results
                row = {**params, **results}
                writer.writerow(row)
            except Exception as e:
                print(f"Error processing file {file_path}: {str(e)}")

if __name__ == "__main__":
    main()