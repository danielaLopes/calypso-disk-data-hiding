#!/bin/bash

echo "------- Plotting security evaluation -------"

# Sort so that it goes in alphabetical order
for f in `ls ./plot_scripts/*.sh | sort -g`; do
  echo "-> Test file: $f"
  bash "$f" 
done

# bash "./plot_scripts/entropy_before_after_usable_blocks.sh"