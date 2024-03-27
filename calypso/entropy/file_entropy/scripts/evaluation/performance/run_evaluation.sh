#!/bin/bash

echo "------- Plotting performance evaluation -------"

for f in ./plot_scripts/*.sh; do
  echo "-> Test file: $f"
  bash "$f" 
done