#! /bin/sh

# To execute all tests: 
#   $ bash run_tests.sh

# dd output explained:
#   a+b records in
#   c+d records out
#       a is for full blocks with bs size
#       b is for blocks with smaller size than the bs
#       c
#       d

CALYPSO_MODULE_MOUNTPOINT="/media/virtual_calypso"

echo "------- Setup -------"
if [[ -f $CALYPSO_MODULE_MOUNTPOINT ]]
then
    echo "Calypso mountpoint exists"
else 
    sudo mkdir $CALYPSO_MODULE_MOUNTPOINT
fi

echo "------- Executing tests -------"

for f in *.bats; do
  echo "-> Test file: $f"
  bats "$f" 
done

#echo "Executing failure tests"

#for f in failure/*.sh; do
 # bash "$f" 
#done