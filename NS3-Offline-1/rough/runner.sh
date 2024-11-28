#!/bin/bash

# Arrays of values for each parameter
# nodeCounts=(20 40 70 100)
# packetsPerSecond=(100 200 300 400)
# nodeSpeeds=(5 10 15 20)

nodeCounts=(20)
packetsPerSecond=(100)
nodeSpeeds=(5)

dir="./scratch/results"

# Create a directory for results if it doesn't exist
mkdir -p ${dir}
rm -rf ${dir}/*

# Get current timestamp for the run
timestamp=$(date +"%Y%m%d_%H%M%S")

# Create a log file
log_file="${dir}/simulation_log_${timestamp}.txt"
echo "Starting simulations at $(date)" > "$log_file"

# Counter for tracking progress
total_runs=$((${#nodeCounts[@]} * ${#packetsPerSecond[@]} * ${#nodeSpeeds[@]}))
current_run=0

# Iterate through all combinations
for count in "${nodeCounts[@]}"; do
    for pps in "${packetsPerSecond[@]}"; do
        for speed in "${nodeSpeeds[@]}"; do
            ((current_run++))
            
            # Create a descriptive name for this run
            run_name="n${count}_p${pps}_s${speed}"
            
            echo "[$current_run/$total_runs] Running simulation with:"
            echo "  Node Count: $count"
            echo "  Packets/s: $pps"
            echo "  Node Speed: $speed"
            
            # Log the start of this run
            echo -e "\n=== Run $run_name started at $(date) ===" >> "$log_file"
            
            # Run the simulation and capture output
            ./ns3 run "scratch/aodv-analysis --nodeCount=$count --packetsPerSecond=$pps --nodeSpeed=$speed" 
            
            # Log the completion
            echo "=== Run $run_name completed at $(date) ===" >> "$log_file"
            echo "Progress: $current_run/$total_runs simulations completed"
            echo -e "\n"
            
            # Optional: add a small delay between runs to prevent system overload
            sleep 1
        done
    done
done

echo "All simulations completed at $(date)" >> "$log_file"
echo "All simulations completed! Check simulation_results directory for outputs."
