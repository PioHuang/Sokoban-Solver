#!/bin/bash

# Output file
output_file="microban.txt"
> "$output_file"  # clear existing output

# Run abc and feed it sokoban commands
./abc > "$output_file" 2>&1 << EOF
$(for i in $(seq 1 50); do
    echo "sokoban filled/microban_${i}.txt 1 0"
done)
EOF
