import xml.etree.ElementTree as ET

def parse_flowmon(file_path):
    # Parse the XML file
    tree = ET.parse(file_path)
    root = tree.getroot()

    # Initialize accumulators
    total_tx_bytes = 0
    total_rx_bytes = 0
    total_tx_packets = 0
    total_rx_packets = 0
    total_lost_packets = 0
    total_delay_sum = 0.0  # Use float to handle scientific notation
    flow_count = 0

    # Iterate over Flow elements
    for flow in root.findall(".//Flow"):
        flow_count += 1

        # Get attributes
        tx_bytes = int(flow.get("txBytes", "0"))
        rx_bytes = int(flow.get("rxBytes", "0"))
        tx_packets = int(flow.get("txPackets", "0"))
        rx_packets = int(flow.get("rxPackets", "0"))
        lost_packets = int(flow.get("lostPackets", "0"))
        delay_sum_ns = float(flow.get("delaySum", "0ns").rstrip("ns"))  # Handle scientific notation

        # Accumulate metrics
        total_tx_bytes += tx_bytes
        total_rx_bytes += rx_bytes
        total_tx_packets += tx_packets
        total_rx_packets += rx_packets
        total_lost_packets += lost_packets
        total_delay_sum += delay_sum_ns

    # Calculate metrics
    network_throughput = (total_rx_bytes * 8) / 50  # Convert bytes to bits and scale to Gbps
    avg_end_to_end_delay = (total_delay_sum / total_rx_packets) / 1e9 if total_rx_packets > 0 else 0  # Convert ns to seconds
    packet_drop_ratio = (total_lost_packets / total_tx_packets) if total_tx_packets > 0 else 0
    packet_delivery_ratio = (total_rx_packets / total_tx_packets) if total_tx_packets > 0 else 0

    # Print results
    print(f"Total Flows: {flow_count}")
    print(f"Network Throughput: {network_throughput:.6f} bps")
    print(f"Average End-to-End Delay: {avg_end_to_end_delay:.6f} seconds")
    print(f"Packet Drop Ratio: {packet_drop_ratio:.6f}")
    print(f"Packet Delivery Ratio: {packet_delivery_ratio:.6f}")

# Example usage
parse_flowmon("./results/task-1/aodv-analysis_nodeCount-20_packetsPerSecond-400_nodeSpeed-15.flowmon")
