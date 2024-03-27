import os
import math
import pathlib
import hashlib


BYTES_IN_A_GB = 1073741824

CUR_DIR = str(pathlib.Path(__file__).parent.resolve()) + "/"

PLAINTEXT_FILES_DIRECTORY = CUR_DIR + "../virtual_disks/plaintext/"
PLAINTEXT_DISK_PATH = "/dev/sdb"

IMAGE_FILES_DIRECTORY = "../virtual_disks/image/"
IMAGE_DISK_PATH = "/dev/sdc"

# VIDEO_FILES_DIRECTORY = CUR_DIR + "../virtual_disks/video/"
# VIDEO_DISK_PATH = "/dev/sdd"

COMPRESSED_FILES_DIRECTORY = CUR_DIR + "../virtual_disks/compressed/"
# COMPRESSED_DISK_PATH = "/dev/sde"
COMPRESSED_DISK_PATH = "/dev/sdd"

ENCRYPTED_FILES_DIRECTORY = CUR_DIR + "../virtual_disks/encrypted/"
# ENCRYPTED_FILE_NAME = "big_encrypted_file"
ENCRYPTED_FILE_NAME = "encrypted2.txt"
# ENCRYPTED_DISK_PATH = "/dev/sdf"
ENCRYPTED_DISK_PATH = "/dev/sde"


def get_file_size(file):
    file.seek(0, os.SEEK_END)
    file_size = file.tell()
    # set back to beginning
    file.seek(0)
    return file_size


def copy_small_files_to_virtual_disk(files_directory, disk_path):
    disk_offset = 0
    with open(disk_path, "wb") as disk_file:
        for file_name in os.listdir(files_directory):
            file_complete_path = os.path.join(files_directory, file_name)
            if not file_name.startswith('.') and os.path.isfile(file_complete_path):
                print("DIRECTORY: " + files_directory + file_name)
                with open(files_directory + file_name, "rb") as file:
                    file_size = get_file_size(file)
                    file_contents = file.read()
                if (disk_offset + file_size > BYTES_IN_A_GB):
                    file_contents = file_contents[ : BYTES_IN_A_GB - disk_offset]
                    disk_file.seek(disk_offset)
                    disk_file.write(file_contents)
                    break
                elif (disk_offset + file_size == BYTES_IN_A_GB):
                    break
                disk_file.seek(disk_offset)
                disk_file.write(file_contents)
                # print("FILE CONTENTS: " + str(file_contents))
                disk_offset += file_size
                # print("DISK OFFSET: " + str(disk_offset))


# read line by line instead of the whole file at once due to being too big
def copy_big_files_to_virtual_disk(files_directory, disk_path):
    disk_offset = 0
    with open(disk_path, "wb") as disk_file:
        for file_name in os.listdir(files_directory):
            file_complete_path = os.path.join(files_directory, file_name)
            if not file_name.startswith('.') and os.path.isfile(file_complete_path):
                print("DIRECTORY: " + file_complete_path)
                with open(files_directory + file_name, "rb") as file:
                    file_size = get_file_size(file)
                    for line in file:
                        line_size = len(line)
                        if (disk_offset + line_size > BYTES_IN_A_GB):
                            line = line[ : BYTES_IN_A_GB - disk_offset]
                            disk_file.seek(disk_offset)
                            disk_file.write(line)
                            break
                        elif (disk_offset + line_size == BYTES_IN_A_GB):
                            break
                        disk_file.seek(disk_offset)
                        disk_file.write(line)
                        disk_offset += line_size
                        # print("DISK OFFSET: " + str(disk_offset))


# to keep in user environment where the links to the libraries are
# $ sudo -E python3 write_files_to_virtual_disk.py
if __name__ == '__main__':
    # first we should format the disk to ensure it is clean
    # copy_small_files_to_virtual_disk(PLAINTEXT_FILES_DIRECTORY, PLAINTEXT_DISK_PATH)
    # copy_small_files_to_virtual_disk(IMAGE_FILES_DIRECTORY, IMAGE_DISK_PATH)

    # # copy_files_to_virtual_disk(VIDEO_FILES_DIRECTORY, VIDEO_DISK_PATH)

    # copy_big_files_to_virtual_disk(COMPRESSED_FILES_DIRECTORY, COMPRESSED_DISK_PATH)
    copy_big_files_to_virtual_disk(ENCRYPTED_FILES_DIRECTORY, ENCRYPTED_DISK_PATH)