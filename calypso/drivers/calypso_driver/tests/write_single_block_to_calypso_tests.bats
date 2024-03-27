#!/usr/bin/env bats

TEST_DATA="/home/daniela/vboxshare/thesis/calypso/drivers/calypso_driver/tests/data/"

FIRST_FREE_BLOCK=12157
SECOND_FREE_BLOCK=12158
THIRD_FREE_BLOCK=12159
FOURTH_FREE_BLOCK=12160
FIFTH_FREE_BLOCK=12161

load 'libs/bats-support/load'
load 'libs/bats-assert/load'
load 'setup_calypso'

#setup_file() {

#}

# executed after all tests
#teardown_file() {
           
#}

# Single block tests
@test "Load Calypso" {
    run setup_calypso
    assert_success

    sudo insmod $CALYPSO_MODULE_PATH blocks=1000 is_clean_start=1
}

# @test "Find first free blocks" {
#     run sudo dumpe2fs $DISK
#     FREE_BLOCK_LINE=${lines[0]}
# }

@test "Write first block of Calypso" {
    # Test functionality
    # run sudo dd if=/home/daniela/vboxshare/thesis/calypso/drivers/calypso_driver/tests/data/first_block seek=0 count=1 bs=4096 of=/dev/calypso0
    run sudo dd if=${TEST_DATA}first_block seek=0 count=1 bs=4096 of=/dev/calypso0
    # Test multi-line output
    assert_line "0+1 records in"
    assert_line "0+1 records out"
    assert_line --partial "11 bytes copied,"
    
    run sudo dd if=/dev/calypso0 skip=0 count=1 bs=4096
    assert_line --partial "FIRST BLOCK"
    assert_line --partial "1+0 records in"
    assert_line "1+0 records out"
    assert_line --partial "4096 bytes"
}

@test "Write middle block of Calypso" {
    # Test functionality
    run sudo dd if=${TEST_DATA}middle_block seek=500 count=1 bs=4096 of=/dev/calypso0
    # Test multi-line output
    assert_line "0+1 records in"
    assert_line "0+1 records out"
    assert_line --partial "12 bytes copied,"
    
    run sudo dd if=/dev/calypso0 skip=500 count=1 bs=4096
    assert_line --partial "MIDDLE BLOCK"
    assert_line --partial "1+0 records in"
    assert_line "1+0 records out"
    assert_line --partial "4096 bytes"
}

@test "Write last block of Calypso" {
    # Test functionality
    run sudo dd if=${TEST_DATA}last_block seek=999 count=1 bs=4096 of=/dev/calypso0
    # Test multi-line output
    assert_line "0+1 records in"
    assert_line "0+1 records out"
    assert_line --partial "10 bytes copied,"
    
    run sudo dd if=/dev/calypso0 skip=999 count=1 bs=4096
    assert_line --partial "LAST BLOCK"
    assert_line --partial "1+0 records in"
    assert_line "1+0 records out"
    assert_line --partial "4096 bytes"
}

@test "Write out-of-range block of Calypso" {
    # Test functionality
    run sudo dd if=${TEST_DATA}last_block seek=1000 count=1 bs=4096 of=/dev/calypso0
    # Test multi-line output
    refute_line "0+1 records in"  || assert_line "dd: error writing '/dev/calypso0': No space left on device"
    refute_line "0+1 records out"
    refute_line --partial "10 bytes copied,"
    
    run sudo dd if=/dev/calypso0 skip=1000 count=1 bs=4096
    refute_line --partial "LAST BLOCK"
    refute_line --partial "1+0 records in"
    refute_line "1+0 records out"
    refute_line --partial "4096 bytes"
}

@test "Write another out-of-range block of Calypso" {
    # Test functionality
    run sudo dd if=${TEST_DATA}last_block seek=1001 count=1 bs=4096 of=/dev/calypso0
    # Test multi-line output
    refute_line "0+1 records in"
    refute_line "0+1 records out"
    refute_line --partial "10 bytes copied,"
    
    run sudo dd if=/dev/calypso0 skip=1001 count=1 bs=4096
    refute_line --partial "LAST BLOCK"
    refute_line --partial "1+0 records in"
    refute_line "1+0 records out"
    refute_line --partial "4096 bytes"
}

@test "Setup reads" {
    run cleanup_calypso
    assert_success
}


# I cannot perform these tests because there is no way of knowing the free block attributed
# But this feature is working

# @test "Read first block of Calypso" {
#     # sudo dd if=/dev/sda8 skip=12157 count=1 bs=4096
#     run sudo dd if=${DISK} skip=${FIRST_FREE_BLOCK} count=1 bs=4096

#     assert_line --partial "FIRST BLOCK"
# }

# @test "Read middle block of Calypso" {
#     run sudo dd if=${DISK} skip=${SECOND_FREE_BLOCK} count=1 bs=4096

#     assert_line --partial "MIDDLE BLOCK"
# }

# @test "Read last block of Calypso" {
#     run sudo dd if=${DISK} skip=${THIRD_FREE_BLOCK} count=1 bs=4096

#     assert_line --partial "LAST BLOCK"
# }

# @test "Fail read first out-of-range block of Calypso" {
#     run sudo dd if=${DISK} skip=${FOURTH_FREE_BLOCK} count=1 bs=4096

#     refute_line --partial "LAST BLOCK"
# }

# @test "Fail read second out-of-range block of Calypso" {
#     run sudo dd if=${DISK} skip=${FIFTH_FREE_BLOCK} count=1 bs=4096

#     refute_line --partial "LAST BLOCK"
# }

@test "Unload Calypso" {
    run sudo rmmod $CALYPSO_MODULE_NAME
    assert_success

    run cleanup_calypso
    assert_success
}