#!/bin/bash

VIRTUAL_DISKS_DIR="../../../virtual_disks/"
SCRIPTS_DIR="../../../scripts/"

RESULTS_FILE="results/entropy_results_baseline2.txt"

DISK_PLAINTEXT="/dev/sdb"
DISK_IMAGE="/dev/sdc"
DISK_COMPRESSED="/dev/sdd"
DISK_ENCRYPTED="/dev/sde"

POPULATE=false

# ---------- Download and setup required datasets ----------

# Gutenberg ebooks dataset
# Cite https://web.eecs.umich.edu/~lahiri/gutenberg_dataset.html
# https://drive.google.com/u/0/uc?id=17WBziFbt9nhAW5iV-yHPHmCfquBPrjJO&export=download
if test -f "${VIRTUAL_DISKS_DIR}Gutenberg.zip"; then 
    echo "# ${VIRTUAL_DISKS_DIR}Gutenberg.zip exists"
else
    echo "# ${VIRTUAL_DISKS_DIR}Gutenberg.zip does not exists! Downloading it!"
    wget --no-check-certificate  -O "${VIRTUAL_DISKS_DIR}Gutenberg.zip" "https://drive.google.com/u/0/uc?id=17WBziFbt9nhAW5iV-yHPHmCfquBPrjJO&export=download"  -r -A 'uc*' -e robots=off -nd
    unzip "${VIRTUAL_DISKS_DIR}Gutenberg.zip" -d "${VIRTUAL_DISKS_DIR}"
    # for file in ${VIRTUAL_DISKS_DIR}plaintext/Gutenberg/*; do mv $file ${VIRTUAL_DISKS_DIR}plaintext/; done
    mv ${VIRTUAL_DISKS_DIR}Gutenberg/txt/* ${VIRTUAL_DISKS_DIR}plaintext/
    rm -rf ${VIRTUAL_DISKS_DIR}Gutenberg
    # rm "${VIRTUAL_DISKS_DIR}Gutenberg.zip"
fi

# Images dataset
# Cite: 
# https://cocodataset.org/#download
if test -f "${VIRTUAL_DISKS_DIR}test2017.zip"; then 
    echo "# ${VIRTUAL_DISKS_DIR}test2017.zip exists"
else
    echo "# ${VIRTUAL_DISKS_DIR}test2017.zip does not exists! Downloading it!"
    wget -O "${VIRTUAL_DISKS_DIR}test2017.zip" "http://images.cocodataset.org/zips/test2017.zip"
    unzip "${VIRTUAL_DISKS_DIR}test2017.zip" -d "${VIRTUAL_DISKS_DIR}"
    # for file in ${VIRTUAL_DISKS_DIR}image/test2017/*; do mv "$file" ${VIRTUAL_DISKS_DIR}image/; done
    mv ${VIRTUAL_DISKS_DIR}test2017/* ${VIRTUAL_DISKS_DIR}image/
    rm -rf ${VIRTUAL_DISKS_DIR}text2017
    # rm "${VIRTUAL_DISKS_DIR}test2017.zip"
    cp "${VIRTUAL_DISKS_DIR}test2017.zip" "${VIRTUAL_DISKS_DIR}compressed"
fi 

# for file in ${VIRTUAL_DISKS_DIR}image/test2017/*; do mv "$file" ${VIRTUAL_DISKS_DIR}image/; done
# rm -rf ${VIRTUAL_DISKS_DIR}image/text2017


# Test file generated with 1Gb
# engineerhammad.com/2015/04/Download-Test-Files.html
if test -f "${VIRTUAL_DISKS_DIR}test1Gb.db"; then 
    echo "# ${VIRTUAL_DISKS_DIR}test1Gb.db exists"
else
    echo "# ${VIRTUAL_DISKS_DIR}test1Gb.db does not exists! Downloading it!"
    # wget -O "~/vboxshare/thesis/calypso/entropy/file_entropy/test1Gb.db" "http://speedtest.ftp.otenet.gr/files/test1Gb.db"
    wget -O "${VIRTUAL_DISKS_DIR}test1Gb.db" "http://speedtest.ftp.otenet.gr/files/test1Gb.db"
    # Encrypt 1 Gb file
    # https://dotlayer.com/how-to-encrypt-decrypt-files-with-openssl-on-ubuntu-and-mac-os-x/
    openssl des3 -in "${VIRTUAL_DISKS_DIR}test1Gb.db" -out "${VIRTUAL_DISKS_DIR}encrypted/encrypted.txt"
fi


# ---------- Populate virtual disks ----------
# Populate virtual disks with dataset files
if [ "$POPULATE" == true ]; then 
    echo "# Populating..."
    sudo -E python3 "${SCRIPTS_DIR}write_files_to_virtual_disk.py"
else
    echo "# Not populating..."
fi

# ---------- Obtain Virtual disks description ----------
sudo -E python3 "${SCRIPTS_DIR}write_virtual_disks_description.py"

# ----------Calculate the entropies of all the blocks in each virtual disk ----------
# gcc -Wall file_entropy_in_disk.c ../../disk_entropy.c -o file_entropy_in_disk -lm
gcc -Wall "${SCRIPTS_DIR}file_entropy_in_disk.c" "${SCRIPTS_DIR}../../disk_entropy.c" -o "${SCRIPTS_DIR}file_entropy_in_disk" -lm
# sudo ./file_entropy_in_disk results/entropy_results_baseline2.txt /dev/sdb /dev/sdc /dev/sdd /dev/sde
sudo "${SCRIPTS_DIR}file_entropy_in_disk" $RESULTS_FILE $DISK_PLAINTEXT $DISK_IMAGE $DISK_COMPRESSED $DISK_ENCRYPTED

# Plot evaluation graphs
python3 "${SCRIPTS_DIR}plot_file_entropy_results.py" $RESULTS_FILE