#!/bin/bash

# Arrays of protocols and variants
protocols=("AODV" "RAODV")
# RAODV variants (0, 1, 2), AODV only uses variant 0
variants=(0)  # Default for AODV

aodv_variants=(0)  # Default for AODV
raodv_variants=(0 1 2)  # Default for AODV

# Arrays of values for each parameter
# nodeCounts=(20)
# packetsPerSecond=(100)
# nodeSpeeds=(5)

# Arrays of values for each parameter
nodeCounts=(20 40 70 100)
packetsPerSecond=(100 200 300 400)
nodeSpeeds=(5 10 15 20)

dir="./scratch/results"

# Create a directory for results if it doesn't exist
mkdir -p ${dir}
rm -rf ${dir}/*

# Get current timestamp for the run
timestamp=$(date +"%Y%m%d_%H%M%S")

# Create a log file
log_file="${dir}/simulation_log_${timestamp}.txt"
echo "Starting simulations at $(date)" > "$log_file"

# Calculate total runs
# For AODV: 1 variant × (nodeCounts + packetsPerSecond + nodeSpeeds)
# For RAODV: 3 variants × (nodeCounts + packetsPerSecond + nodeSpeeds)
runs_per_set=$((${#nodeCounts[@]} + ${#packetsPerSecond[@]} + ${#nodeSpeeds[@]}))
total_runs=$((runs_per_set * (${#aodv_variants[@]} + ${#raodv_variants[@]}))) 
current_run=0

for protocol in "${protocols[@]}"; do
    # Set variants based on protocol
    if [ "$protocol" == "AODV" ]; then
        variants=("${aodv_variants[@]}")
    else
        variants=("${raodv_variants[@]}")
    fi
    
    for variant in "${variants[@]}"; do
        echo "=== Running simulations for $protocol (variant $variant) ===" >> "$log_file"
        
        # Run experiments varying node count
        echo "=== Running Node Count Experiments ===" >> "$log_file"
        for count in "${nodeCounts[@]}"; do
            ((current_run++))
            run_name="${protocol}_v${variant}_nodeCount_${count}"
            
            echo "[$current_run/$total_runs] Running simulation with Protocol: $protocol, Variant: $variant, Node Count: $count"
            echo -e "\n=== Run $run_name started at $(date) ===" >> "$log_file"
            
            ./ns3 run "scratch/aodv-raodv-analysis --comment=nodeCount --protocolName=$protocol --variant=$variant --nodeCount=$count"
            
            echo "=== Run $run_name completed at $(date) ===" >> "$log_file"
            echo "Progress: $current_run/$total_runs simulations completed"
            echo -e "\n"
            sleep 1
        done

        # Run experiments varying packets per second
        echo "=== Running Packets Per Second Experiments ===" >> "$log_file"
        for pps in "${packetsPerSecond[@]}"; do
            ((current_run++))
            run_name="${protocol}_v${variant}_packetsPerSecond_${pps}"
            
            echo "[$current_run/$total_runs] Running simulation with Protocol: $protocol, Variant: $variant, Packets/s: $pps"
            echo -e "\n=== Run $run_name started at $(date) ===" >> "$log_file"
            
            ./ns3 run "scratch/aodv-raodv-analysis --comment=packetsPerSecond --protocolName=$protocol --variant=$variant --packetsPerSecond=$pps"
            
            echo "=== Run $run_name completed at $(date) ===" >> "$log_file"
            echo "Progress: $current_run/$total_runs simulations completed"
            echo -e "\n"
            sleep 1
        done

        # Run experiments varying node speed
        echo "=== Running Node Speed Experiments ===" >> "$log_file"
        for speed in "${nodeSpeeds[@]}"; do
            ((current_run++))
            run_name="${protocol}_v${variant}_nodeSpeed_${speed}"
            
            echo "[$current_run/$total_runs] Running simulation with Protocol: $protocol, Variant: $variant, Node Speed: $speed"
            echo -e "\n=== Run $run_name started at $(date) ===" >> "$log_file"
            
            ./ns3 run "scratch/aodv-raodv-analysis --comment=nodeSpeed --protocolName=$protocol --variant=$variant --nodeSpeed=$speed"
            
            echo "=== Run $run_name completed at $(date) ===" >> "$log_file"
            echo "Progress: $current_run/$total_runs simulations completed"
            echo -e "\n"
            sleep 1
        done
    done
done

echo "All simulations completed at $(date)" >> "$log_file"
echo "All simulations completed! Check simulation_results directory for outputs."