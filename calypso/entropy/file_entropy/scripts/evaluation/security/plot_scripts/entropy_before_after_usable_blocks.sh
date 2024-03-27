#!/bin/bash

# SCRIPTS_DIR="../../../scripts/"
SCRIPTS_DIR="/home/daniela/vboxshare/thesis/calypso/entropy/file_entropy/scripts/"

RESULTS_FILE_BEFORE="results/entropy_results_baseline4.txt"

DISK_PLAINTEXT="/dev/sdf"
DISK_IMAGE="/dev/sdg"
DISK_COMPRESSED="/dev/sdh"
DISK_ENCRYPTED="/dev/sdi"


# for entropy_threshold in {0..7};
# do
#     RESULTS_FILE_AFTER="results/entropy_results_before_after_usable_blocks_threshold_${entropy_threshold}.txt"
#     PLOT_FILE_BEFORE="plots/usable_blocks_entropy_intervals_plot_${entropy_threshold}_threshold_before.pdf"
#     PLOT_FILE_AFTER="plots/usable_blocks_entropy_intervals_plot_${entropy_threshold}_threshold_after.pdf"

#     if test -f "${SCRIPTS_DIR}${RESULTS_FILE_AFTER}"; then 
#         echo "# ${SCRIPTS_DIR}${RESULTS_FILE_AFTER} exists"
#     else
#         echo "# ${SCRIPTS_DIR}${RESULTS_FILE_AFTER} does not exists! Generating it will take some time! Generating..."
#         # Populate virtual disks with dataset files and blocks with entropy value above the given entropy threshold
#         sudo -E python3 "${SCRIPTS_DIR}encrypt_blocks_above_entropy.py" $RESULTS_FILE_BEFORE $entropy_threshold

#         # ----------Calculate the entropies of all the blocks in each virtual disk ----------
#         gcc -Wall "${SCRIPTS_DIR}file_entropy_in_disk.c" "${SCRIPTS_DIR}../../disk_entropy.c" -o "${SCRIPTS_DIR}file_entropy_in_disk" -lm
#         sudo "${SCRIPTS_DIR}file_entropy_in_disk" $RESULTS_FILE_AFTER $DISK_PLAINTEXT $DISK_IMAGE $DISK_COMPRESSED $DISK_ENCRYPTED
#     fi

#     # Plot evaluation graphs with entropy value
#     python3 "${SCRIPTS_DIR}plot_entropy_before_after_usable_blocks.py" $RESULTS_FILE_BEFORE $RESULTS_FILE_AFTER $entropy_threshold $PLOT_FILE_BEFORE $PLOT_FILE_AFTER
# done


for entropy_threshold in {0..7};
# for entropy_threshold in {5..7};
do
    RESULTS_FILE_AFTER="results/entropy_results_after_threshold_${entropy_threshold}.txt"
    
    # if test -f "${SCRIPTS_DIR}${RESULTS_FILE_AFTER}"; then 
    #     echo "# ${SCRIPTS_DIR}${RESULTS_FILE_AFTER} exists"
    # else
        # echo "# ${SCRIPTS_DIR}${RESULTS_FILE_AFTER} does not exists! Generating it will take some time! Generating..."
        # Populate virtual disks with dataset files and blocks with entropy value above the given entropy threshold
        # gcc -Wall file_entropy_in_disk.c ../../disk_entropy.c -o file_entropy_in_disk -lm
        sudo -E python3 "${SCRIPTS_DIR}encrypt_blocks_above_entropy.py" $RESULTS_FILE_BEFORE $entropy_threshold

        # ----------Calculate the entropies of all the blocks in each virtual disk ----------
        # sudo ./file_entropy_in_disk results/entropy_results_after_threshold_5.txt /dev/sdf /dev/sdg /dev/sdh /dev/sdi
        sudo "${SCRIPTS_DIR}file_entropy_in_disk" $RESULTS_FILE_AFTER $DISK_PLAINTEXT $DISK_IMAGE $DISK_COMPRESSED $DISK_ENCRYPTED
    # fi

done


# Plot evaluation graphs with entropy value
    python3 "${SCRIPTS_DIR}plot_entropy_before_after_usable_blocks.py"