#!/bin/bash

# Function to preserve capitalization while replacing
replace_with_case() {
    local search=$1
    local replace=$2
    local str=$3
    
    # Handle different case patterns without word boundaries
    echo "$str" | sed "s/$search/$replace/g" \
                | sed "s/$(echo $search | tr '[:lower:]' '[:upper:]')/$(echo $replace | tr '[:lower:]' '[:upper:]')/g" \
                | sed "s/$(echo $search | sed 's/.*/\L&/; s/^./\u&/')/$(echo $replace | sed 's/.*/\L&/; s/^./\u&/')/g"
}

# Directory containing AODV files
AODV_DIR="./aodv"

# First rename contents of files
find "$AODV_DIR" -type f -exec grep -l -i "aodv" {} \; | while read file; do
    # Create temp file
    temp_file=$(mktemp)
    
    # Replace contents preserving case
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