#!/usr/bin/env bats

TOTAL_BLOCK_COUNT=300000
HELLO_STR="hello"

TOR_VERSION=10.5

load 'libs/bats-support/load'
load 'libs/bats-assert/load'
load 'setup_calypso'


# Tests for installing and using Tor in our shadow partition
@test "Load Calypso" {
    run setup_calypso
    assert_success

    run sudo insmod $CALYPSO_MODULE_PATH blocks=${TOTAL_BLOCK_COUNT} is_clean_start=1
    assert_success
}

@test "Format Calypso with ext4" {
    run bash -c "yes | sudo mkfs -t ext4 /dev/calypso0"
    assert_success 

    assert_line --partial "Creating filesystem with $TOTAL_BLOCK_COUNT 4k blocks and "
    assert_line --partial "Allocating group tables"
    assert_line --partial "Writing inode tables"
    assert_line --partial "Writing superblocks and filesystem accounting information"
}

@test "Mount Calypso with ext4, install and execute Tor browser" {
    run sudo mount /dev/calypso0 /media/virtual_calypso
    assert_success

    # Download Tor browser after removing previous version
    rm /home/daniela/Downloads/tor-browser-linux64-${TOR_VERSION}_en-US.tar.xz
    run wget --directory-prefix=/home/daniela/Downloads/ https://www.torproject.org/dist/torbrowser/${TOR_VERSION}/tor-browser-linux64-${TOR_VERSION}_en-US.tar.xz
    assert_success

    cd /media
    # Set permissions for folder
    run sudo chmod -R -v 777 virtual_calypso
    run sudo chmod -R -v 777 virtual_calypso/*
    assert_success
    cd /media/virtual_calypso
    run cp /home/daniela/Downloads/tor-browser-linux64-${TOR_VERSION}_en-US.tar.xz .
    assert_success

    run tar -xf tor-browser-linux64-${TOR_VERSION}_en-US.tar.xz
    assert_success

    cd /media # exit mount point so that it can umount

    /media/virtual_calypso/tor-browser_en-US/Browser/start-tor-browser &
    process_id=$! # gets PID of last job run in background
    wait $process_id # waits for all background processes to finish

    run sudo umount /media/virtual_calypso
    assert_success
}

@test "Unload Calypso" {
    run sudo rmmod calypso_driver
    assert_success
}

@test "Setup to recover Tor" {
    run cleanup_calypso
    assert_success

    run sudo insmod $CALYPSO_MODULE_PATH blocks=${TOTAL_BLOCK_COUNT}
    assert_success
}

@test "Recover Tor" {
    run sudo mount /dev/calypso0 /media/virtual_calypso
    assert_success
    
    cd /media
    # Set permissions for folder
    run sudo chmod -R -v 777 virtual_calypso
    run sudo chmod -R -v 777 virtual_calypso/*

    /media/virtual_calypso/tor-browser_en-US/Browser/start-tor-browser &
    process_id_recovered=$! # gets PID of last job run in background
    wait $process_id_recovered # waits for all background processes to finish

    # close Tor browser
    # probably takes a little while to be able to umount
    run sudo umount /media/virtual_calypso
    assert_success
}

@test "Unload Calypso after recovering Tor" {
    run sudo rmmod calypso_driver
    assert_success
}
