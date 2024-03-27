#!/usr/bin/env bats

HELLO_STR="hello"

TOTAL_BLOCK_COUNT=1000

load 'libs/bats-support/load'
load 'libs/bats-assert/load'
load 'setup_calypso'


# Test if mappings are recovered correctly
@test "Save block mappings in file" {
    run setup_calypso
    assert_success

    # sudo insmod /home/daniela/vboxshare/thesis/calypso/drivers/calypso_driver/calypso_driver.ko blocks=1000 is_clean_start=1
    run sudo insmod $CALYPSO_MODULE_PATH blocks=${TOTAL_BLOCK_COUNT} is_clean_start=1
    assert_success

    run sudo dd if=data/first_block bs=4096 count=1 seek=5 of=/dev/calypso0
    assert_line --partial "0+1 records in"
    assert_line --partial "0+1 records out"
    assert_line --partial "11 bytes copied,"

    sleep 1 # we need to wait or module reports as in use
    run sudo rmmod calypso_driver
    assert_success

    run cleanup_calypso
    assert_success

    # sudo insmod /home/daniela/vboxshare/thesis/calypso/drivers/calypso_driver/calypso_driver.ko blocks=1000 
    run sudo insmod $CALYPSO_MODULE_PATH blocks=${TOTAL_BLOCK_COUNT}
    assert_success

    run sudo dd if=/dev/calypso0 bs=4096 count=1 skip=5
    assert_line --partial "FIRST BLOCK"
    assert_line --partial "1+0 records in"
    assert_line --partial "1+0 records out"

    sleep 1 # we need to wait or module reports as in use
    run sudo rmmod calypso_driver
    assert_success

    run cleanup_calypso
    assert_success
}


# Persistency tests with recovering mounts
@test "Mount ext4 with information recovered from persistency" {
    # sudo insmod /home/daniela/vboxshare/thesis/calypso/drivers/calypso_driver/calypso_driver.ko blocks=1000 is_clean_start=1
    run sudo insmod $CALYPSO_MODULE_PATH blocks=${TOTAL_BLOCK_COUNT} is_clean_start=1
    assert_success
    run bash -c "yes | sudo mkfs -t ext4 /dev/calypso0"
    assert_success 

    assert_line --partial "Creating filesystem with $TOTAL_BLOCK_COUNT 4k blocks and "
    assert_line --partial "Allocating group tables"
    assert_line --partial "Writing inode tables"
    assert_line --partial "Writing superblocks and filesystem accounting information"

    run sudo mount /dev/calypso0 /media/virtual_calypso
    assert_success
    run sudo mkdir /media/virtual_calypso/dir
    assert_success

    echo $HELLO_STR > hello.txt
    run sudo cp hello.txt /media/virtual_calypso/dir/
    assert_success
    rm hello.txt

    run sudo umount /media/virtual_calypso
    assert_success

    run sudo rmmod calypso_driver
    assert_success

    run cleanup_calypso
    assert_success

    run sudo insmod $CALYPSO_MODULE_PATH blocks=${TOTAL_BLOCK_COUNT}
    assert_success

    run sudo mount /dev/calypso0 /media/virtual_calypso
    assert_success

    run sudo cat /media/virtual_calypso/dir/hello.txt
    assert_success
    assert_output $HELLO_STR

    run sudo umount /media/virtual_calypso
    assert_success

    run sudo rmmod calypso_driver
    assert_success

    run cleanup_calypso
    assert_success
}

# Persistency tests for failure with recovering mounts
@test "Fail mount ext4 with information recovered from persistency" {
    # sudo insmod /home/daniela/vboxshare/thesis/calypso/drivers/calypso_driver/calypso_driver.ko blocks=1000 is_clean_start=1
    run sudo insmod $CALYPSO_MODULE_PATH blocks=${TOTAL_BLOCK_COUNT} is_clean_start=1
    assert_success
    run bash -c "yes | sudo mkfs -t ext4 /dev/calypso0"
    assert_success 

    assert_line --partial "Creating filesystem with $TOTAL_BLOCK_COUNT 4k blocks and "
    assert_line --partial "Allocating group tables"
    assert_line --partial "Writing inode tables"
    assert_line --partial "Writing superblocks and filesystem accounting information"

    run sudo mount /dev/calypso0 /media/virtual_calypso
    assert_success
    run sudo mkdir /media/virtual_calypso/dir
    assert_success

    echo $HELLO_STR > hello.txt
    run sudo cp hello.txt /media/virtual_calypso/dir/
    assert_success
    rm hello.txt

    run sudo umount /media/virtual_calypso
    assert_success

    run sudo rmmod calypso_driver
    assert_success

    run cleanup_calypso
    assert_success

    run sudo insmod $CALYPSO_MODULE_PATH blocks=${TOTAL_BLOCK_COUNT} is_clean_start=1
    assert_success

    run sudo mount /dev/calypso0 /media/virtual_calypso
    assert_failure

    run sudo rmmod calypso_driver
    assert_success

    run cleanup_calypso
    assert_success
}