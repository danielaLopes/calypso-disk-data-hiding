#!/bin/bash

echo "------- Plotting isolation evaluation -------"

# for f in ./plot_scripts/*.sh; do
#   echo "-> Test file: $f"
#   bash "$f" 
# done

HOME_DIR="/home/daniela/"
TOR_STRING="tor-browser"


sudo find / -mmin -5 -ls > files_changed_before_tor_baseline.txt
sudo grep -r -l $TOR_STRING / > strings_after_tor_baseline.txt

cp ~/Downloads/tor-browser-linux64-10.5_en-US.tar.xz $HOME_DIR
tar -xf ${HOME_DIR}tor-browser-linux64-10.5_en-US.tar.xz
${HOME_DIR}tor-browser_en-US/Browser/start-tor-browser &

# Execute part of utilization script

sudo find / -mmin -10 -ls > files_changed_after_tor_baseline.txt
# TODO: Not working, see this
sudo grep -r -l $TOR_STRING / > strings_after_tor_baseline.txt