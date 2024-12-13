#!/bin/bash

rm -rf ./scratch/results/*

# Array of TCP protocols to test
protocols=(
    "TcpVeno"
    "TcpWestwoodPlus"
    "TcpLogWestwoodPlus"
    "TcpStochasticLogWestwoodPlus"
)

# Simulation duration in seconds
DURATION=100

# Create base results directory if it doesn't exist
BASE_DIR="./scratch/results"
mkdir -p "$BASE_DIR"

# Run simulation for each protocol
for protocol in "${protocols[@]}"; do
    echo "Running simulation for $protocol..."
    
    # Create protocol-specific results directory
    PROTOCOL_DIR="$BASE_DIR/$protocol"
    mkdir -p "$PROTOCOL_DIR"
    
    # Run the simulation
    ./ns3 run "scratch/tcp-variants-comparison --num_flows=7 --transport_prot=$protocol --duration=$DURATION"
    
    # Move results to protocol directory if needed
    # Uncomment and modify these lines if results are generated in a different location
    # mv ./scratch/results/*.data "$PROTOCOL_DIR/"
    
    echo "Completed simulation for $protocol"
    echo "----------------------------------------"
done

echo "All simulations completed!"