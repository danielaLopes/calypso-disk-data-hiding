#!/usr/bin/env bats

CALYPSO_MODULE_NAME="calypso_driver"
CALYPSO_MODULE_PATH="/home/daniela/vboxshare/thesis/calypso/drivers/calypso_driver/calypso_driver.ko"
CALYPSO_MODULE_MOUNTPOINT="/media/virtual_calypso"
CALYPSO_BITMAP_PERSISTENCE_FILE="/home/daniela/vboxshare/thesis/calypso/drivers/calypso_driver/calypso_bitmap.dat"
CALYPSO_MAPPINGS_PERSISTENCE_FILE="/home/daniela/vboxshare/thesis/calypso/drivers/calypso_driver/calypso_mappings.dat"

DISK=/dev/sda8

FIRST_FREE_BLOCK=12157
SECOND_FREE_BLOCK=12158

DATA="Calypso has written into this block!"
DATA_OVERWRITE="Data has been overwritten!"
NEW_DATA="Check if block mappings are still correct!"

TOTAL_BLOCK_COUNT=1000

load 'libs/bats-support/load'
load 'libs/bats-assert/load'
load 'setup_calypso'


# Test cleaning caches first
@test "Clean caches initial test" {
    # put random data in blocks before loading Calypso
    assert_equal $FIRST_FREE_BLOCK 12157
    assert_equal $SECOND_FREE_BLOCK 12158
    sudo dd if=/dev/random count=1 bs=4096 seek=$FIRST_FREE_BLOCK of=$DISK
    sudo dd if=/dev/random count=1 bs=4096 seek=$SECOND_FREE_BLOCK of=$DISK

    run setup_calypso
    assert_success

    run sudo insmod $CALYPSO_MODULE_PATH blocks=${TOTAL_BLOCK_COUNT}
    assert_success

    echo $DATA > text.txt    
    sudo dd if=text.txt count=1 bs=4096 seek=0 of=/dev/calypso0
    rm text.txt

    # read non updated value
    run sudo dd if=$DISK count=1 bs=4096 skip=$FIRST_FREE_BLOCK
    assert_success
    refute_line --partial $DATA

    sync
    sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
    sudo blockdev --flushbufs $DISK
    sudo hdparm -F $DISK

    # read updated value
    run sudo dd if=$DISK count=1 bs=4096 skip=$FIRST_FREE_BLOCK
    assert_success
    assert_line --partial $DATA

    run sudo rmmod $CALYPSO_MODULE_NAME
    assert_success

    rm $CALYPSO_BITMAP_PERSISTENCE_FILE
    rm $CALYPSO_MAPPINGS_PERSISTENCE_FILE
}


# Tests that overwrite 1 Calypso block and demand Calypso 
# to copy to to the next free block

# @test "Find first 2 free blocks" {
    #run sudo dumpe2fs $DISK > free_blocks.txt
    #assert_success

    #read FIRST_FREE_BLOCK <<< $(awk '/Free blocks:/ {print $1}' free_blocks.txt)
    #FIRST_FREE_BLOCK=$(awk '/Free blocks:/ {print $1}' free_blocks.txt)
    #assert_equal $FIRST_FREE_BLOCK "12157"

    #run bash -c "echo $FIRST_FREE_BLOCK"
    #run bash -c "echo \"hello\""
    #SECOND_FREE_BLOCK=$FIRST_FREE_BLOCK + 1
    #echo SECOND_FREE_BLOCK
    #rm free_blocks.txt

# }

@test "Clean first 2 free blocks" {
    # put random data in blocks before loading Calypso
    sudo dd if=/dev/random count=1 bs=4096 seek=$FIRST_FREE_BLOCK of=$DISK
    sudo dd if=/dev/random count=1 bs=4096 seek=$SECOND_FREE_BLOCK of=$DISK
}

@test "Load Calypso with $TOTAL_BLOCK_COUNT blocks" {
    run sudo insmod $CALYPSO_MODULE_PATH blocks=${TOTAL_BLOCK_COUNT}
    assert_success
}

@test "Write to block and override" {
    echo $DATA > text.txt 
    # since we cleared the mappings, this is going to overwrite the first free block
    sudo dd if=text.txt count=1 bs=4096 seek=0 of=/dev/calypso0
    rm text.txt

    # actual overwrite
    echo $DATA_OVERWRITE > text.txt 
    sudo dd if=text.txt count=1 bs=4096 seek=$FIRST_FREE_BLOCK of=$DISK
    rm text.txt
}

@test "Read blocks before cleaning caches" {
    # read non updated values, first one was already updated
    run sudo dd if=$DISK count=1 bs=4096 skip=$FIRST_FREE_BLOCK
    assert_success
    assert_line --partial $DATA_OVERWRITE

    run sudo dd if=$DISK count=1 bs=4096 skip=$SECOND_FREE_BLOCK
    assert_success
    refute_line --partial $DATA
}

@test "Clean caches" {
    sync
    sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
    sudo blockdev --flushbufs $DISK
    sudo hdparm -F $DISK
}

@test "Read blocks after caches are cleaned" {
    # read updated values
    run sudo dd if=$DISK count=1 bs=4096 skip=$FIRST_FREE_BLOCK
    assert_success
    assert_line --partial $DATA_OVERWRITE

    run sudo dd if=$DISK count=1 bs=4096 skip=$SECOND_FREE_BLOCK
    assert_success
    assert_line --partial $DATA
}

# test if mappings are correct after copying
@test "Test if mappings are correct after copying a block" {
    echo $NEW_DATA > text.txt    
    sudo dd if=text.txt count=1 bs=4096 seek=0 of=/dev/calypso0
    rm text.txt

    sync
    sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
    sudo blockdev --flushbufs $DISK
    sudo hdparm -F $DISK

    # read updated values
    run sudo dd if=$DISK count=1 bs=4096 skip=$FIRST_FREE_BLOCK
    assert_success
    assert_line --partial $DATA_OVERWRITE

    run sudo dd if=$DISK count=1 bs=4096 skip=$SECOND_FREE_BLOCK
    assert_success
    assert_line --partial $NEW_DATA
}

@test "Unload Calypso with $TOTAL_BLOCK_COUNT blocks" {
    run sudo rmmod $CALYPSO_MODULE_NAME
    assert_success

    rm $CALYPSO_BITMAP_PERSISTENCE_FILE
    rm $CALYPSO_MAPPINGS_PERSISTENCE_FILE
}


# Test copying for more than 1 block