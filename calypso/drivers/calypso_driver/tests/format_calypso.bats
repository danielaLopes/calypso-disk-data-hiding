#!/usr/bin/env bats

TOTAL_BLOCK_COUNT_SMALL=1000
TOTAL_BLOCK_COUNT_MEDIUM=100000
TOTAL_BLOCK_COUNT_BIG=300000
TOTAL_BLOCK_COUNT_1GB=262144 # 1Gb
TOTAL_BLOCK_COUNT_2GB=524288 # 2Gb
TOTAL_BLOCK_COUNT_3GB=786432 # 3Gb
HELLO_STR="hello"

load 'libs/bats-support/load'
load 'libs/bats-assert/load'
load 'setup_calypso'


# Tests for small block count
@test "Load Calypso with $TOTAL_BLOCK_COUNT_SMALL blocks" {
    run setup_calypso
    assert_success

    run sudo insmod $CALYPSO_MODULE_PATH blocks=${TOTAL_BLOCK_COUNT_SMALL} is_clean_start=1
    assert_success
}

@test "Format Calypso with ext4 with $TOTAL_BLOCK_COUNT_SMALL blocks" {
    run bash -c "yes | sudo mkfs -t ext4 /dev/calypso0"
    assert_success 

    assert_line --partial "Creating filesystem with $TOTAL_BLOCK_COUNT_SMALL 4k blocks and "
    assert_line --partial 'Allocating group tables:'
    assert_line --partial "Writing inode tables:"
    assert_line --partial "Writing superblocks and filesystem accounting information:"
 
    # ensure the integrity of the file system
    run sudo fsck.ext4 -fn /dev/calypso0
    assert_success
    assert_line "Pass 1: Checking inodes, blocks, and sizes"
    assert_line "Pass 2: Checking directory structure"
    assert_line "Pass 3: Checking directory connectivity"
    assert_line "Pass 4: Checking reference counts"
    assert_line "Pass 5: Checking group summary information"
}

@test "Mount Calypso with ext4 with $TOTAL_BLOCK_COUNT_SMALL blocks" {
    run sudo mount /dev/calypso0 /media/virtual_calypso
    assert_success
    run sudo mkdir "$CALYPSO_MODULE_MOUNTPOINT/dir"
    assert_success

    echo $HELLO_STR > hello.txt
    run sudo cp hello.txt "$CALYPSO_MODULE_MOUNTPOINT/dir"
    assert_success
    rm hello.txt

    run sudo umount $CALYPSO_MODULE_MOUNTPOINT
    assert_success

    run cleanup_calypso
    assert_success

    run sudo mount /dev/calypso0 $CALYPSO_MODULE_MOUNTPOINT
    assert_success
    run sudo cat "$CALYPSO_MODULE_MOUNTPOINT/dir/hello.txt"
    assert_success
    assert_output $HELLO_STR
  
    run sudo umount $CALYPSO_MODULE_MOUNTPOINT
    assert_success
}

@test "Format Calypso with FAT with $TOTAL_BLOCK_COUNT_SMALL blocks" {
    if mountpoint -q $CALYPSO_MODULE_MOUNTPOINT; then
        run sudo umount $CALYPSO_MODULE_MOUNTPOINT
        assert_success 
    fi
    run bash -c "yes | sudo mkfs -t vfat -b 4096 /dev/calypso0"
    assert_success

    # ensure the integrity of the file system
    run sudo fsck.vfat -fn /dev/calypso0
    assert_success
}

@test "Mount Calypso with FAT with $TOTAL_BLOCK_COUNT_SMALL blocks" {
    run sudo mount /dev/calypso0 /media/virtual_calypso
    assert_success
    run sudo mkdir "$CALYPSO_MODULE_MOUNTPOINT/dir"
    assert_success

    echo $HELLO_STR > hello.txt
    run sudo cp hello.txt "$CALYPSO_MODULE_MOUNTPOINT/dir"
    assert_success
    rm hello.txt

    run sudo umount $CALYPSO_MODULE_MOUNTPOINT
    assert_success

    run cleanup_calypso
    assert_success

    run sudo mount /dev/calypso0 $CALYPSO_MODULE_MOUNTPOINT
    assert_success
    run sudo cat "$CALYPSO_MODULE_MOUNTPOINT/dir/hello.txt"
    assert_success
    assert_output $HELLO_STR
  
    run sudo umount $CALYPSO_MODULE_MOUNTPOINT
    assert_success
}

@test "Format Calypso with NTFS with $TOTAL_BLOCK_COUNT_SMALL blocks" {
    if mountpoint -q $CALYPSO_MODULE_MOUNTPOINT; then
        run sudo umount $CALYPSO_MODULE_MOUNTPOINT
        assert_success 
    fi
    run bash -c "yes | sudo mkfs -t ntfs -c 4096 /dev/calypso0"
    assert_success

    # ensure the integrity of the file system
    run sudo ntfsfix -n /dev/calypso0
    assert_success
    assert_line "Mounting volume... OK"
    assert_line "Processing of \$MFT and \$MFTMirr completed successfully."
    assert_line "Checking the alternate boot sector... OK"
    assert_line "NTFS partition /dev/calypso0 was processed successfully."
}

@test "Mount Calypso with NTFS with $TOTAL_BLOCK_COUNT_SMALL blocks" {
    run sudo mount /dev/calypso0 /media/virtual_calypso
    assert_success
    run sudo mkdir "$CALYPSO_MODULE_MOUNTPOINT/dir"
    assert_success

    echo $HELLO_STR > hello.txt
    run sudo cp hello.txt "$CALYPSO_MODULE_MOUNTPOINT/dir"
    assert_success
    rm hello.txt

    run sudo umount $CALYPSO_MODULE_MOUNTPOINT
    assert_success

    run cleanup_calypso
    assert_success

    run sudo mount /dev/calypso0 $CALYPSO_MODULE_MOUNTPOINT
    assert_success
    run sudo cat "$CALYPSO_MODULE_MOUNTPOINT/dir/hello.txt"
    assert_success
    assert_output $HELLO_STR
  
    run sudo umount $CALYPSO_MODULE_MOUNTPOINT
    assert_success
}

@test "Unload Calypso with $TOTAL_BLOCK_COUNT_SMALL blocks" {
    run sudo rmmod $CALYPSO_MODULE_NAME
    assert_success
}


# Tests for medium block count
@test "Load Calypso with $TOTAL_BLOCK_COUNT_MEDIUM blocks" {
    sudo insmod $CALYPSO_MODULE_PATH blocks=${TOTAL_BLOCK_COUNT_MEDIUM} is_clean_start=1
}

@test "Format Calypso with ext4 with $TOTAL_BLOCK_COUNT_MEDIUM blocks" {
    if mountpoint -q $CALYPSO_MODULE_MOUNTPOINT; then
        run sudo umount $CALYPSO_MODULE_MOUNTPOINT
        assert_success 
    fi
    run bash -c "yes | sudo mkfs -t ext4 /dev/calypso0"
    assert_success 

    assert_line --partial "Creating filesystem with $TOTAL_BLOCK_COUNT_MEDIUM 4k blocks and "
    assert_line --partial "Allocating group tables:"
    assert_line --partial "Writing inode tables:"
    assert_line --partial "Writing superblocks and filesystem accounting information:"
 
    # ensure the integrity of the file system
    run sudo fsck.ext4 -fn /dev/calypso0
    assert_success
    assert_line "Pass 1: Checking inodes, blocks, and sizes"
    assert_line "Pass 2: Checking directory structure"
    assert_line "Pass 3: Checking directory connectivity"
    assert_line "Pass 4: Checking reference counts"
    assert_line "Pass 5: Checking group summary information"
}

@test "Mount Calypso with ext4 with $TOTAL_BLOCK_COUNT_MEDIUM blocks" {
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

    run cleanup_calypso
    assert_success

    run sudo mount /dev/calypso0 /media/virtual_calypso
    assert_success
    run sudo cat /media/virtual_calypso/dir/hello.txt
    assert_success
    assert_output $HELLO_STR
  
    run sudo umount /media/virtual_calypso
    assert_success
}

@test "Format Calypso with FAT with $TOTAL_BLOCK_COUNT_MEDIUM blocks" {
    if mountpoint -q $CALYPSO_MODULE_MOUNTPOINT; then
        run sudo umount $CALYPSO_MODULE_MOUNTPOINT
        assert_success 
    fi
    run bash -c "yes | sudo mkfs -t vfat -b 4096 /dev/calypso0"
    assert_success

    # ensure the integrity of the file system
    run sudo fsck.vfat -fn /dev/calypso0
    assert_success
}

@test "Mount Calypso with FAT with $TOTAL_BLOCK_COUNT_MEDIUM blocks" {
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

    run cleanup_calypso
    assert_success

    run sudo mount /dev/calypso0 /media/virtual_calypso
    assert_success
    run sudo cat /media/virtual_calypso/dir/hello.txt
    assert_success
    assert_output $HELLO_STR
  
    run sudo umount /media/virtual_calypso
    assert_success
}

@test "Format Calypso with NTFS with $TOTAL_BLOCK_COUNT_MEDIUM blocks" {
    if mountpoint -q $CALYPSO_MODULE_MOUNTPOINT; then
        run sudo umount $CALYPSO_MODULE_MOUNTPOINT
        assert_success 
    fi
    run bash -c "yes | sudo mkfs -t ntfs -c 4096 /dev/calypso0"
    assert_success

    # ensure the integrity of the file system
    run sudo ntfsfix -n /dev/calypso0
    assert_success
    assert_line "Mounting volume... OK"
    assert_line "Processing of \$MFT and \$MFTMirr completed successfully."
    assert_line "Checking the alternate boot sector... OK"
    assert_line "NTFS partition /dev/calypso0 was processed successfully."
}

@test "Mount Calypso with NTFS with $TOTAL_BLOCK_COUNT_MEDIUM blocks" {
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

    run cleanup_calypso
    assert_success

    run sudo mount /dev/calypso0 /media/virtual_calypso
    assert_success
    run sudo cat /media/virtual_calypso/dir/hello.txt
    assert_success
    assert_output $HELLO_STR
  
    run sudo umount /media/virtual_calypso
    assert_success
}

@test "Unload Calypso with $TOTAL_BLOCK_COUNT_MEDIUM blocks" {
    run sudo rmmod $CALYPSO_MODULE_NAME
    assert_success
}


# Tests for big block count
@test "Load Calypso with $TOTAL_BLOCK_COUNT_BIG blocks" {
    sudo insmod $CALYPSO_MODULE_PATH blocks=${TOTAL_BLOCK_COUNT_BIG} is_clean_start=1
}

@test "Format Calypso with ext4 with $TOTAL_BLOCK_COUNT_BIG blocks" {
    if mountpoint -q $CALYPSO_MODULE_MOUNTPOINT; then
        run sudo umount $CALYPSO_MODULE_MOUNTPOINT
        assert_success 
    fi
    run bash -c "yes | sudo mkfs -t ext4 /dev/calypso0"
    assert_success 

    assert_line --partial "Creating filesystem with $TOTAL_BLOCK_COUNT_BIG 4k blocks and "
    assert_line --partial "Allocating group tables:"
    assert_line --partial "Writing inode tables:"
    assert_line --partial "Writing superblocks and filesystem accounting information:"
 
    # ensure the integrity of the file system
    run sudo fsck.ext4 -fn /dev/calypso0
    assert_success
    assert_line "Pass 1: Checking inodes, blocks, and sizes"
    assert_line "Pass 2: Checking directory structure"
    assert_line "Pass 3: Checking directory connectivity"
    assert_line "Pass 4: Checking reference counts"
    assert_line "Pass 5: Checking group summary information"
}

@test "Mount Calypso with ext4 with $TOTAL_BLOCK_COUNT_BIG blocks" {
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

    run cleanup_calypso
    assert_success

    run sudo mount /dev/calypso0 /media/virtual_calypso
    assert_success
    run sudo cat /media/virtual_calypso/dir/hello.txt
    assert_success
    assert_output $HELLO_STR
  
    run sudo umount /media/virtual_calypso
    assert_success
}

# @test "Format Calypso with FAT with $TOTAL_BLOCK_COUNT_BIG blocks" {
#     if mountpoint -q $CALYPSO_MODULE_MOUNTPOINT; then
#         run sudo umount $CALYPSO_MODULE_MOUNTPOINT
#         assert_success 
#     fi
#     run bash -c "yes | sudo mkfs -t vfat -b 4096 /dev/calypso0"
#     assert_success

#     # ensure the integrity of the file system
#     run sudo fsck.vfat -fn /dev/calypso0
#     assert_success
# }

# @test "Mount Calypso with FAT with $TOTAL_BLOCK_COUNT_BIG blocks" {
#     run sudo mount /dev/calypso0 $CALYPSO_MODULE_MOUNTPOINT
#     assert_success
#     run sudo mkdir "$CALYPSO_MODULE_MOUNTPOINT/dir"
#     assert_success

#     echo $HELLO_STR > hello.txt
#     run sudo cp hello.txt "$CALYPSO_MODULE_MOUNTPOINT/dir"
#     assert_success
#     rm hello.txt

#     run sudo umount $CALYPSO_MODULE_MOUNTPOINT
#     assert_success

#     run sudo mount /dev/calypso0 $CALYPSO_MODULE_MOUNTPOINT
#     assert_success
#     run sudo cat "$CALYPSO_MODULE_MOUNTPOINT/dir/hello.txt"
#     assert_success
#     assert_output $HELLO_STR
  
#     run sudo umount $CALYPSO_MODULE_MOUNTPOINT
#     assert_success
# }

@test "Format Calypso with NTFS with $TOTAL_BLOCK_COUNT_BIG blocks" {
    if mountpoint -q $CALYPSO_MODULE_MOUNTPOINT; then
        run sudo umount $CALYPSO_MODULE_MOUNTPOINT
        assert_success 
    fi
    run bash -c "yes | sudo mkfs -t ntfs -c 4096 /dev/calypso0"
    assert_success

    # ensure the integrity of the file system
    run sudo ntfsfix -n /dev/calypso0
    assert_success
    assert_line "Mounting volume... OK"
    assert_line "Processing of \$MFT and \$MFTMirr completed successfully."
    assert_line "Checking the alternate boot sector... OK"
    assert_line "NTFS partition /dev/calypso0 was processed successfully."
}

@test "Mount Calypso with NTFS with $TOTAL_BLOCK_COUNT_BIG blocks" {
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

    run cleanup_calypso
    assert_success

    run sudo mount /dev/calypso0 /media/virtual_calypso
    assert_success
    run sudo cat /media/virtual_calypso/dir/hello.txt
    assert_success
    assert_output $HELLO_STR
  
    run sudo umount /media/virtual_calypso
    assert_success
}

@test "Unload Calypso with $TOTAL_BLOCK_COUNT_BIG blocks" {
    run sudo rmmod $CALYPSO_MODULE_NAME
    assert_success
}


# Tests for 1 Gb block count
@test "Load Calypso with $TOTAL_BLOCK_COUNT_1GB blocks" {
    run sudo insmod $CALYPSO_MODULE_PATH blocks=${TOTAL_BLOCK_COUNT_1GB} is_clean_start=1
    assert_success
}

@test "Format Calypso with ext4 with $TOTAL_BLOCK_COUNT_1GB blocks" {
    if mountpoint -q $CALYPSO_MODULE_MOUNTPOINT; then
        run sudo umount $CALYPSO_MODULE_MOUNTPOINT
        assert_success 
    fi
    run bash -c "yes | sudo mkfs -t ext4 /dev/calypso0"
    assert_success 

    assert_line --partial "Creating filesystem with $TOTAL_BLOCK_COUNT_1GB 4k blocks and "
    assert_line --partial 'Allocating group tables:'
    assert_line --partial "Writing inode tables:"
    assert_line --partial "Writing superblocks and filesystem accounting information:"
 
    # ensure the integrity of the file system
    run sudo fsck.ext4 -fn /dev/calypso0
    assert_success
    assert_line "Pass 1: Checking inodes, blocks, and sizes"
    assert_line "Pass 2: Checking directory structure"
    assert_line "Pass 3: Checking directory connectivity"
    assert_line "Pass 4: Checking reference counts"
    assert_line "Pass 5: Checking group summary information"
}

@test "Mount Calypso with ext4 with $TOTAL_BLOCK_COUNT_1GB blocks" {
    run sudo mount /dev/calypso0 /media/virtual_calypso
    assert_success
    run sudo mkdir "$CALYPSO_MODULE_MOUNTPOINT/dir"
    assert_success

    echo $HELLO_STR > hello.txt
    run sudo cp hello.txt "$CALYPSO_MODULE_MOUNTPOINT/dir"
    assert_success
    rm hello.txt

    run sudo umount $CALYPSO_MODULE_MOUNTPOINT
    assert_success

    run cleanup_calypso
    assert_success

    run sudo mount /dev/calypso0 $CALYPSO_MODULE_MOUNTPOINT
    assert_success
    run sudo cat "$CALYPSO_MODULE_MOUNTPOINT/dir/hello.txt"
    assert_success
    assert_output $HELLO_STR
  
    run sudo umount $CALYPSO_MODULE_MOUNTPOINT
    assert_success
}

@test "Unload Calypso with $TOTAL_BLOCK_COUNT_1GB blocks" {
    run sudo rmmod $CALYPSO_MODULE_NAME
    assert_success
}