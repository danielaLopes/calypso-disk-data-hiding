CALYPSO_MODULE_NAME="calypso_driver"
CALYPSO_MODULE_PATH="/home/daniela/vboxshare/thesis/calypso/drivers/calypso_driver/calypso_driver.ko"
CALYPSO_MODULE_MOUNTPOINT="/media/virtual_calypso"
CALYPSO_BITMAP_PERSISTENCE_FILE="/home/daniela/vboxshare/thesis/calypso/drivers/calypso_driver/calypso_bitmap.dat"
CALYPSO_MAPPINGS_PERSISTENCE_FILE="/home/daniela/vboxshare/thesis/calypso/drivers/calypso_driver/calypso_mappings.dat"

DISK=/dev/sda8


function setup_calypso() {
    echo "# ----- Setup before each test file -----" >&3

    # Create mount point if necessary
    if [[ -f $CALYPSO_MODULE_MOUNTPOINT ]]
    then
        echo "# $CALYPSO_MODULE_MOUNTPOINT exists on your filesystem" >&3
    else
        sudo mkdir $CALYPSO_MODULE_MOUNTPOINT
    fi

    # Umounts Calypso partition if necessary
    if mountpoint -q $CALYPSO_MODULE_MOUNTPOINT; then
        echo "# $CALYPSO_MODULE_MOUNTPOINT is mounted, unmounting ..." >&3
        run sudo umount $CALYPSO_MODULE_MOUNTPOINT
        assert_success 
    fi

    # Removes Calypso module if necessary
    if lsmod | grep "$CALYPSO_MODULE_NAME" &> /dev/null ; 
    then
        echo "# Calypso module is loaded, unloading it ..." >&3
        run sudo rmmod $CALYPSO_MODULE_NAME 
        assert_success
    fi
}


function cleanup_calypso() {
    echo "# ----- Cleanup after each test file -----" >&3
    
    # Clear buffer caches so it will read the mos updated value from disk
    sync
    sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
    sudo blockdev --flushbufs $DISK
    sudo hdparm -F $DISK
}