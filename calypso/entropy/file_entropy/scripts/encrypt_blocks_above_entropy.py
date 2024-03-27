import os
import sys
import math
import pathlib
from random import randint

# import pyaes, pbkdf2, binascii, os, secrets


# password = "s3cr3t*c0d3"
# passwordSalt = os.urandom(16)
# key = pbkdf2.PBKDF2(password, passwordSalt).read(32)


BYTES_IN_A_GB = 1073741824

CUR_DIR = str(pathlib.Path(__file__).parent.resolve()) + "/"

PLAINTEXT_FILES_DIRECTORY = CUR_DIR + "../virtual_disks/plaintext/"
PLAINTEXT_DISK_PATH_FROM = "/dev/sdb"
PLAINTEXT_DISK_PATH_TO = "/dev/sdf"

IMAGE_FILES_DIRECTORY = "../virtual_disks/image/"
IMAGE_DISK_PATH_FROM = "/dev/sdc"
IMAGE_DISK_PATH_TO = "/dev/sdg"

COMPRESSED_FILES_DIRECTORY = CUR_DIR + "../virtual_disks/compressed/"
COMPRESSED_DISK_PATH_FROM = "/dev/sdd"
COMPRESSED_DISK_PATH_TO = "/dev/sdh"

ENCRYPTED_FILES_DIRECTORY = CUR_DIR + "../virtual_disks/encrypted/"
# ENCRYPTED_FILE_NAME = "encrypted.txt"
ENCRYPTED_FILE_NAME = "encrypted2.txt"
ENCRYPTED_DISK_PATH_FROM = "/dev/sde"
ENCRYPTED_DISK_PATH_TO = "/dev/sdi"


def read_random_encrypted_block():
    with open(ENCRYPTED_FILES_DIRECTORY + ENCRYPTED_FILE_NAME, "rb") as encrypted_file:
        random_block = randint(0, BYTES_IN_A_GB / 4096 - 1)
        encrypted_file.seek(random_block * 4096)
        # encrypted_file.seek(100 * 4096)
        # print("RANDOM BLOCK: " + str(random_block))
        return encrypted_file.read(4096)

# def encrypt_block(block_plaintext):
#     iv = secrets.randbits(256)
#     aes = pyaes.AESModeOfOperationCTR(key, pyaes.Counter(iv))
#     ciphertext = aes.encrypt(block_plaintext)
#     # print(ciphertext)
#     return ciphertext


# read block by block and encrypt blocks above a given thresholdinstead of the whole file at once due to being too big
def copy_and_encrypt_block_to_virtual_disk(results_path, disk_path_from, disk_path_to, entropy_threshold, results_file_start_line):
    disk_offset = 0
    block_index = 0
    block_entropy = None

    read_block = None
    write_block = None

    print("Writing encrypted blocks to: " + disk_path_to)
    with open(disk_path_from, "rb") as disk_file_from, open(disk_path_to, "wb") as disk_file_to, open(results_path, "r") as results_file:
        line_counter = results_file_start_line
        # Advance to right part of file
        while line_counter > 0:
            results_file.readline()
            line_counter -= 1
        
        # Advance first line (we don't care about this result)
        block_entropy = results_file.readline()
        print("first relevant line: " + block_entropy)

        # encrypted_block = read_random_encrypted_block()
        # encrypted_block = encrypt_block(block_plaintext)

        while True:
            # Read disk block by block
            read_block = disk_file_from.read(4096)
            # encrypted_block = read_random_encrypted_block()
            # print("encrypted_block: " + encrypted_block)
            parsed_entropy = results_file.readline().split('\n')[0]
            block_entropy = float(parsed_entropy)
            # print("ENTROPY : " + str(block_entropy))
            # print("ENTROPY THRESHOLD : " + str(entropy_threshold))

            # Encrypt blocks that have higher entropy than the threshold one
            if (block_entropy >= entropy_threshold):
                # if disk_path_from == IMAGE_DISK_PATH_FROM:
                #     print("THRESHOLD: " + str(entropy_threshold))
                #     print("block entropy: " + str(block_entropy))
                # if disk_path_from == IMAGE_DISK_PATH_FROM:
                #     print("ENCRYPTING BLOCK WITH ENTROPY: " + str(block_entropy))
                write_block = read_random_encrypted_block()
                # write_block = read_random_encrypted_block()
                # print("ENCRYPTED WRITE BLOCK: " + str(write_block[0 : 10]))
            else:
                # print("MAINTAINING BLOCK WITH ENTROPY :" + str(block_entropy))
                # if disk_path_from == IMAGE_DISK_PATH_FROM:
                #     print("not ENCRYPTING BLOCK WITH ENTROPY: " + str(block_entropy))
                write_block = read_block

            if (disk_offset + 4096 >= BYTES_IN_A_GB):
                disk_file_to.seek(disk_offset)
                disk_file_to.write(write_block)
                print("BREAK DISK OFFSET: " + str(disk_offset))
                break
            disk_file_to.seek(disk_offset)
            disk_file_to.write(write_block)
            disk_offset += 4096
            block_index += 1
            # print("DISK OFFSET: " + str(disk_offset))


# to keep in user environment where the links to the libraries are
# $ sudo -E python3 encrypt_blocks_above_entropy.py "results/entropy_results_baseline4.txt" 0
if __name__ == '__main__':
    results_path = CUR_DIR + sys.argv[1]
    entropy_threshold = int(sys.argv[2])

    copy_and_encrypt_block_to_virtual_disk(results_path, PLAINTEXT_DISK_PATH_FROM, PLAINTEXT_DISK_PATH_TO, entropy_threshold, 0)
    copy_and_encrypt_block_to_virtual_disk(results_path, IMAGE_DISK_PATH_FROM, IMAGE_DISK_PATH_TO, entropy_threshold, 262145)
    copy_and_encrypt_block_to_virtual_disk(results_path, COMPRESSED_DISK_PATH_FROM, COMPRESSED_DISK_PATH_TO, entropy_threshold, 524290)
    copy_and_encrypt_block_to_virtual_disk(results_path, ENCRYPTED_DISK_PATH_FROM, ENCRYPTED_DISK_PATH_TO, entropy_threshold, 786435)
