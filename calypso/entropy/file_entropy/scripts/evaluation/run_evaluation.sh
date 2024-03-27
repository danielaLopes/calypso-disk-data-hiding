#!/bin/bash

echo "------- Running EVALUATION scripts -------"

declare -a domains=("isolation" "functionality" "performance" "security" "storage_capacity")

#for domain in ./*/run_evaluation.sh; do
for domain in ${domains[@]}; do
  echo "-> Running script files for each evaluation domain: $domain"
  bash "$domain/run_evaluation.sh" 
done