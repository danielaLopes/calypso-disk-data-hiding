#!/usr/bin/env bats

TEST_DATA="/home/daniela/vboxshare/thesis/calypso/drivers/calypso_driver/tests/big_file"

TOTAL_BLOCK_COUNT=1000
MIDDLE_BLOCK_COUNT=$(($TOTAL_BLOCK_COUNT/2))
# Change next two values to number of blocks in the sequence
BLOCK_COUNT_AUX=4
BLOCK_COUNT="4"
BYTE_COUNT_AUX=$(($BLOCK_COUNT_AUX*4096))
BYTE_COUNT="${BYTE_COUNT_AUX}"
LAST_BLOCK=$(($TOTAL_BLOCK_COUNT-$BLOCK_COUNT_AUX))

load 'libs/bats-support/load'
load 'libs/bats-assert/load'
load 'setup_calypso'

@test "Load Calypso" {
    run setup_calypso
    assert_success

    sudo insmod $CALYPSO_MODULE_PATH blocks=${TOTAL_BLOCK_COUNT} is_clean_start=1
}

@test "Create big file" {
    python3 -c "print('a' * 4096 * ${BLOCK_COUNT})" > big_file
}

@test "Write block sequence of ${BLOCK_COUNT} blocks starting on first of Calypso" {
    # Test functionality
    run sudo dd if=${TEST_DATA} seek=0 count=${BLOCK_COUNT} bs=4096 of=/dev/calypso0
    # Test multi-line output
    assert_line "${BLOCK_COUNT}+0 records in"
    assert_line "${BLOCK_COUNT}+0 records out"
    assert_line --partial "${BYTE_COUNT} bytes"
    
    run sudo dd if=/dev/calypso0 skip=0 count=${BLOCK_COUNT} bs=4096
    assert_line --partial "a" * ${BYTE_COUNT_AUX}
    assert_line --partial "${BLOCK_COUNT}+0 records in"
    assert_line "${BLOCK_COUNT}+0 records out"
    assert_line --partial "${BYTE_COUNT} bytes"
}

@test "Write block sequence of ${BLOCK_COUNT} blocks starting on middle block of Calypso" {
    # Test functionality
    run sudo dd if=${TEST_DATA} seek=${MIDDLE_BLOCK_COUNT} count=${BLOCK_COUNT} bs=4096 of=/dev/calypso0
    # Test multi-line output
    assert_line "${BLOCK_COUNT}+0 records in"
    assert_line "${BLOCK_COUNT}+0 records out"
    assert_line --partial "${BYTE_COUNT} bytes"
    
    run sudo dd if=/dev/calypso0 skip=${MIDDLE_BLOCK_COUNT} count=${BLOCK_COUNT} bs=4096
    assert_line --partial "a" * ${BYTE_COUNT_AUX}
    assert_line --partial "${BLOCK_COUNT}+0 records in"
    assert_line "${BLOCK_COUNT}+0 records out"
    assert_line --partial "${BYTE_COUNT} bytes"
}

@test "Write block sequence of ${BLOCK_COUNT} blocks on last ${BLOCK_COUNT} blocks of Calypso" {
    # Test functionality
    run sudo dd if=${TEST_DATA} seek=${LAST_BLOCK} count=${BLOCK_COUNT} bs=4096 of=/dev/calypso0
    # Test multi-line output
    assert_line "${BLOCK_COUNT}+0 records in"
    assert_line "${BLOCK_COUNT}+0 records out"
    assert_line --partial "${BYTE_COUNT} bytes"
    
    run sudo dd if=/dev/calypso0 skip=${LAST_BLOCK} count=${BLOCK_COUNT} bs=4096
    assert_line --partial "a" * ${BYTE_COUNT_AUX}
    assert_line --partial "${BLOCK_COUNT}+0 records in"
    assert_line "${BLOCK_COUNT}+0 records out"
    assert_line --partial "${BYTE_COUNT} bytes"
}

@test "Write out-of-range block sequence of ${BLOCK_COUNT} blocks of Calypso" {
    # Test functionality
    run sudo dd if=${TEST_DATA} seek=${TOTAL_BLOCK_COUNT} count=${BLOCK_COUNT} bs=4096 of=/dev/calypso0
    # Test multi-line output
    assert_line "dd: error writing '/dev/calypso0': No space left on device"
    refute_line "${BLOCK_COUNT}+0 records in"
    refute_line "${BLOCK_COUNT}+0 records out"
    assert_line --partial "0 bytes"
    
    run sudo dd if=/dev/calypso0 skip=${TOTAL_BLOCK_COUNT} count=${BLOCK_COUNT} bs=4096
    refute_line --partial "a" * ${BYTE_COUNT_AUX}
    assert_line --partial "0+0 records in"
    assert_line "0+0 records out"
    assert_line --partial "0 bytes"
}

@test "Write block sequence of 3 blocks with 1 block in range and 2 out-of-range starting of Calypso" {
    # Test functionality
    run sudo dd if=${TEST_DATA} seek=$(($TOTAL_BLOCK_COUNT-1)) count=${BLOCK_COUNT} bs=4096 of=/dev/calypso0
    # Test multi-line output
    assert_line "2+0 records in"
    assert_line "1+0 records out"
    assert_line --partial "4096 bytes"
    
    run sudo dd if=/dev/calypso0 skip=$(($TOTAL_BLOCK_COUNT-1)) count=${BLOCK_COUNT} bs=4096
    assert_line --partial "a" * 4096
    assert_line --partial "1+0 records in"
    assert_line "1+0 records out"
    assert_line --partial "4096 bytes"
}

@test "Write to all Calypso blocks" {
    python3 -c "print('b' * 4096 * ${TOTAL_BLOCK_COUNT})" > calypso_file
    # Test functionality
    run sudo dd if=calypso_file seek=0 count=$TOTAL_BLOCK_COUNT bs=4096 of=/dev/calypso0
    # Test multi-line output
    assert_line "${TOTAL_BLOCK_COUNT}+0 records in"
    assert_line "${TOTAL_BLOCK_COUNT}+0 records out"
    assert_line --partial "$(($TOTAL_BLOCK_COUNT*4096)) bytes"
    
    run sudo dd if=/dev/calypso0 skip=0 count=$TOTAL_BLOCK_COUNT bs=4096
    assert_line --partial "b" * $(($TOTAL_BLOCK_COUNT*4096))
    assert_line --partial "${TOTAL_BLOCK_COUNT}+0 records in"
    assert_line "${TOTAL_BLOCK_COUNT}+0 records out"
    assert_line --partial "$(($TOTAL_BLOCK_COUNT*4096)) bytes"
}

@test "Write to all Calypso blocks with 10 out-of-range blocks" {
    python3 -c "print('b' * 4096 * $(($TOTAL_BLOCK_COUNT+10)))" > calypso_file
    # Test functionality
    run sudo dd if=calypso_file seek=0 count=$(($TOTAL_BLOCK_COUNT+10)) bs=4096 of=/dev/calypso0
    # Test multi-line output
    assert_line "dd: error writing '/dev/calypso0': No space left on device"
    assert_line "$(($TOTAL_BLOCK_COUNT+1))+0 records in"
    assert_line "${TOTAL_BLOCK_COUNT}+0 records out"
    assert_line --partial "$(($TOTAL_BLOCK_COUNT*4096)) bytes"
    
    run sudo dd if=/dev/calypso0 skip=0 count=$(($TOTAL_BLOCK_COUNT+10)) bs=4096
    
    assert_line --partial "b" * $(($TOTAL_BLOCK_COUNT*4096))
    assert_line --partial "${TOTAL_BLOCK_COUNT}+0 records in"
    assert_line "${TOTAL_BLOCK_COUNT}+0 records out"
    assert_line --partial "$(($TOTAL_BLOCK_COUNT*4096)) bytes"
}

@test "Remove auxiliary files" {
    rm big_file
    rm calypso_file
}

@test "Unload Calypso" {
    run sudo rmmod $CALYPSO_MODULE_NAME
    assert_success
}
