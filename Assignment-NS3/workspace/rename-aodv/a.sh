#!/bin/bash

# copy the whole aodv folder here
# then rename references
# then copy the new raodv folder back to ns3-dev/src

# Function to preserve capitalization while replacing, with special cases
replace_with_case() {
    local search=$1
    local replace=$2
    local str=$3
    
    # First protect AODVTYPE_ by temporarily replacing it
    str=$(echo "$str" | sed 's/AODVTYPE_/TEMPORARYTYPE_/g')
    
    # Handle "Aodv" → "RAodv" case specifically
    str=$(echo "$str" | sed 's/Aodv/RAodv/g')
    
    # Handle other cases
    str=$(echo "$str" | sed "s/aodv/raodv/g" \
                     | sed "s/AODV/RAODV/g")
    
    # Restore protected AODVTYPE_
    str=$(echo "$str" | sed 's/TEMPORARYTYPE_/AODVTYPE_/g')
    
    echo "$str"
}

# Directory containing AODV files
AODV_DIR="./aodv"

# First rename contents of files
find "$AODV_DIR" -type f -exec grep -l -i "aodv" {} \; | while read file; do
    # Create temp file
    temp_file=$(mktemp)
    
    # Replace contents preserving case and handling special cases
    replace_with_case "aodv" "raodv" "$(cat "$file")" > "$temp_file"
    
    # Move temp file back to original
    mv "$temp_file" "$file"
done

# Then rename files and directories (bottom-up to handle nested paths)
find "$AODV_DIR" -depth -name "*[aA][oO][dD][vV]*" | while read item; do
    dir=$(dirname "$item")
    old_name=$(basename "$item")
    new_name=$(replace_with_case "aodv" "raodv" "$old_name")
    mv "$item" "$dir/$new_name"
done

cp ../raodv-starter/src_aodv_helper/* ./raodv/helper
cp ../raodv-starter/src_aodv_model/* ./raodv/model