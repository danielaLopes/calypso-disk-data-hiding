from math import ceil
import re
from random import seed
from random import random
from random import randint
import hashlib
import base64
import sys
import os
from cryptography.fernet import Fernet
from cryptography.fernet import InvalidToken
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.backends import default_backend


# https://cryptography.io/en/latest/fernet/#using-passwords-with-fernet

# -------------------------------------------------------
# ----------------- Hardcoded constants ----------------- 
# -------------------------------------------------------
ENCODING = 'latin-1'
SALT = b'F\x18\xad$\xbd\xc2\xa5\xd9d.\xd1\xc0\x92\x1b)\xb4'
RANDOM_NUM_UPPER_LIMIT = 100000000

BLOCK_BYTES = 4096
#BLOCK_BYTES = int(BLOCK_BYTES * 3 / 4) # due to base64 performed by Fernet increasing
# TODO: understand with it needs to be 8, with less the cipher_text length is 4108, with 8 or more it is 4088 < 4096
BLOCK_BYTES = int(BLOCK_BYTES * 3 / 4) - 8 # due to base64 performed by Fernet increasing
print("BLOCK_BYTES " + str(BLOCK_BYTES))
BASE64_BYTES = ceil(BLOCK_BYTES / 3) * 4
print("BASE64_BYTES " + str(BASE64_BYTES))

# Fernet structure
VERSION_BYTES_LEN = 1
TIMESTAMP_BYTES_LEN = 8
SALT_BYTES_LEN = 16
HMAC_BYTES_LEN = 32
DATA_BYTES_LEN = BLOCK_BYTES - (VERSION_BYTES_LEN + TIMESTAMP_BYTES_LEN + SALT_BYTES_LEN + HMAC_BYTES_LEN)
if (DATA_BYTES_LEN % 16) > 0:
    DATA_BYTES_LEN -= DATA_BYTES_LEN % 16 # due to PKCS7 padding
print("DATA_BYTES: " + str(DATA_BYTES_LEN))

VERSION_START = 0
TIMESTAMP_START = VERSION_START + TIMESTAMP_BYTES_LEN
SALT_START = TIMESTAMP_START + SALT_BYTES_LEN
DATA_START = SALT_START + DATA_BYTES_LEN
HMAC_START = -HMAC_BYTES_LEN

# Data structure
MAGIC_NUMBER_BYTES_LEN = 1
NEXT_BLOCK_BYTES_LEN = 8 # sizeof(unsigned long)
METADATA_BYTES_LEN = DATA_BYTES_LEN - (MAGIC_NUMBER_BYTES_LEN + NEXT_BLOCK_BYTES_LEN)
print("METADATA_BYTES_LEN " + str(METADATA_BYTES_LEN))
MAGIC_NUMBER_START = 0
METADATA_START = MAGIC_NUMBER_START + MAGIC_NUMBER_BYTES_LEN
print("METADATA_START " + str(METADATA_START))
NEXT_BLOCK_START = METADATA_START + METADATA_BYTES_LEN

magic_number = chr(int('10111011', 2)) # TODO: is the magic number hardcoded?


# -------------------------------------------------------
# ------------------- Global variables ------------------ 
# -------------------------------------------------------
# this dictionary simulates the blocks in disk
ciphered_blocks = {} # key: block_num, value: ciphered_content
bitmap = ""


# -------------------------------------------------------
# ------------------- Helper functions ------------------ 
# -------------------------------------------------------
def parse_bitmap_file():
    global bitmap 
    #with open("../drivers/calypso_driver/persistence_debug/bitmap_file", 'rb') as bitmap_file:
    with open("../drivers/calypso_driver/persistence_debug/bitmap_file", 'r') as bitmap_file:
        lines = bitmap_file.readlines()
        for line in lines:
            bits_part = line[10:]
            #full_pattern = re.compile('[a-z][A-Z]\s[2-9]')
            #bits = re.sub(full_pattern, '', bits_part)
            #print("BITS: " + str(bits_part))
            bits = bits_part.replace(' ','').replace('.','')
            bits = re.sub('\r?\n', '', bits)
            bitmap += bits


def hash_block_contents(encoded_data):
    m = hashlib.sha256()
    m.update(encoded_data)
    return m.digest()


def generate_key_from_password(password):
    encoded_password = password.encode(ENCODING)
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=32,
        salt=SALT,
        iterations=100000,
        backend=default_backend()
    )
    key = base64.urlsafe_b64encode(kdf.derive(encoded_password))
    return Fernet(key)


def encrypt_block_contents(key, encoded_data):
    cipher_text = key.encrypt(encoded_data)
    #print("\nlen(cipher_text): " + str(len(cipher_text)))
    #print("\nlen(encoded_data): " + str(len(encoded_data)))
    #print("Version:\t",cipher_text[VERSION_START : VERSION_START + VERSION_BYTES_LEN])
    #print("Time stamp:\t",cipher_text[TIMESTAMP_START : TIMESTAMP_START + TIMESTAMP_BYTES_LEN])
    #print("IV:\t\t",cipher_text[SALT_START : SALT_START + SALT_BYTES_LEN])
    #print("DATA_START:\t\t",str(DATA_START))
    #print("HMAC_START:\t\t",str(HMAC_START))
    #print("Data:\t\t",str(cipher_text[DATA_START: HMAC_START]))

    #print("Data len:\t\t", str(len(cipher_text[DATA_START : HMAC_START])))
    #print("HMAC:\t\t", cipher_text[HMAC_START :])
    #return key.encrypt(encoded_data)
    return cipher_text


def decrypt_block_contents(key, ciphertext):
    try:
        plain_text = key.decrypt(ciphertext)
        #print("plain_text block contents len: " + str(len(plain_text)))
        return plain_text # returns only the data that we care about: magic_number + metadata + next_block
        # return key.decrypt(ciphertext)
    except InvalidToken:
        print("Block does not match hash!")


# pass num as -1 to obtain every single random number, 
# which is going to take super long
# or pass a positive number to obtain a smaller amount of numbers
def generate_sequence_of_random_block_numbers_with_seed(password, num, total_blocks_number):
    generated = 0
    random_block_numbers = [] # first five are: 896622, 13849, 24287, 525905, 398812, 661807

    split_password = password.split("-")
    seed_val = int(split_password[0])
    seed(seed_val) 

    first_random_number = randint(0, RANDOM_NUM_UPPER_LIMIT)
    first_random_block_number = first_random_number % total_blocks_number
    print("random block number: {}".format(first_random_block_number))
    random_block_numbers.append(first_random_block_number)

    random_number = -1

    if num == -1:
        while (random_number != first_random_number):
            random_number = randint(0, RANDOM_NUM_UPPER_LIMIT)
            random_block_number = random_number % total_blocks_number
            print("random block number: {}".format(random_block_number))
            random_block_numbers.append(random_block_number)
            generated += 1
    else:
        while (random_number != first_random_number) and (generated < num):
            random_number = randint(0, RANDOM_NUM_UPPER_LIMIT)
            random_block_number = random_number % total_blocks_number
            print("random block number: {}".format(random_block_number))
            if (random_block_number in random_block_numbers):
                print("REPEATED!!!")
            random_block_numbers.append(random_block_number)
            generated += 1

    return random_block_numbers



def set_bit_in_bitmap(block_num):
    global bitmap
    bitmap = bitmap[: block_num] + "1" + bitmap[block_num + 1 :]


def get_next_free_block():
    global bitmap 

    for random_physical_block in range(0, len(bitmap)):
        if bitmap[random_physical_block] == "0":
            # keep track of used blocks
            set_bit_in_bitmap(random_physical_block)
            return random_physical_block


def get_next_random_block(seed_val, total_physical_blocks):
    global bitmap 

    random_num = randint(0, RANDOM_NUM_UPPER_LIMIT)
    random_physical_block = random_num % total_physical_blocks
    first_random_block = random_physical_block

    # We want to keep trying to find a random unused block
    while bitmap[random_physical_block] == "1":
        random_num = randint(0, RANDOM_NUM_UPPER_LIMIT)
        random_physical_block = random_num % total_physical_blocks
        # This means we have looped and we need to advance to the next seed
        if (random_physical_block == first_random_block):
            print("RESETTING SEED")
            seed_val += 1
            seed(val_val)
    # keep track of used blocks
    set_bit_in_bitmap(random_physical_block)

    return (random_physical_block, first_random_block)


def get_next_block(metadata_block_num, seed_val, total_physical_blocks):
    if metadata_block_num == 0:
        return get_next_random_block(seed_val, total_physical_blocks)
    else: 
        return get_next_free_block()


def get_num_needed_blocks(data_len):
    k = 0
    while data_len > 0:
        #data_len -= METADATA_BYTES_LEN * (k + 1)
        data_len -= METADATA_BYTES_LEN
        k += 1
    return k


def write_block(block_num, ciphered_block_contents):
    ciphered_blocks[block_num] = ciphered_block_contents


def read_block(block_num):
    if block_num in ciphered_blocks:
        return ciphered_blocks[block_num]
    else:
        print()

# -------------------------------------------------------
# -------------------- Main functions ------------------- 
# -------------------------------------------------------
def hide_metadata_block(password, key, metadata, total_physical_blocks, metadata_block_num, seed_val, next_block, is_last_iter):
    
    if metadata_block_num == 0:
        (random_physical_block, first_random_block) = get_next_block(metadata_block_num, seed_val, total_physical_blocks)
    else:
        random_physical_block = next_block
        #(random_physical_block, bitmap) = get_next_block(physical_blocks_bitmap, metadata_block_num, seed_val)

    # get number of next block to be used to write
    # Needs to be done before encoding the data in the block so we already know the next block number
    if not is_last_iter:
        next_block = get_next_block(metadata_block_num + 1, seed_val, total_physical_blocks)
    else:
        next_block = -1

    # Takes care of padding if necessary
    metadata_padding_len = METADATA_BYTES_LEN - len(metadata)
    if (metadata_padding_len > 0):
        metadata = (metadata + "{}").format(metadata_padding_len * ' ')
    next_block_str = str(next_block) # TODO: maybe put this as base64 or something that occupies less bytes
    next_block_padding_len = NEXT_BLOCK_BYTES_LEN - len(next_block_str)
    if (next_block_padding_len > 0):
        next_block_str = (next_block_str + "{}").format(next_block_padding_len * ' ')

    encoded_block_contents = (magic_number + metadata + next_block_str).encode(ENCODING)
    # hashed_contents = hash_block_contents(encoded_block_contents)
    # block_contents = encoded_block_contents + hashed_contents
    # ciphered_block_contents = encrypt_block_contents(key, block_contents)
    ciphered_block_contents = encrypt_block_contents(key, encoded_block_contents)
    write_block(random_physical_block, ciphered_block_contents)
    
    #print("hide_metadata_block() random_physical_block: " + str(random_physical_block))

    if metadata_block_num == 0:
        return (next_block, first_random_block)
    elif not is_last_iter:
        return next_block
    else:
        return next_block


def retrieve_metadata_block(password, key, total_physical_blocks, metadata_block_num, seed_val, first_random_block, next_block):
    j = 0

    # TODO: create an ultimate stop condition to stop searching???
    while True:
        # This part is only needed for the first block, in the other cases we already know the block number
        if metadata_block_num == 0:
            random_num = randint(0, RANDOM_NUM_UPPER_LIMIT)
            random_physical_block = random_num % total_physical_blocks

            # This means we have looped and we need to advance to the next seed
            if random_physical_block == first_random_block and j > 0:
                seed_val += 1
                seed(seed_val)
            j += 1 # so that we know if it is already repeating random numbers
        else:
            random_physical_block = next_block

        ciphered_block_contents = read_block(random_physical_block)

        # this is cheating because we will already know when the block is not used, 
        # we are always supposed to retrieve something and check if it is valid
        if ciphered_block_contents is None and metadata_block_num == 0:
            continue # advance to next random block
        block_contents = decrypt_block_contents(key, ciphered_block_contents)
        if block_contents is None: # this means an exception was raising during decryption, so this block is not valid
            break
        decoded_block_contents = block_contents.decode(ENCODING)
        
        read_magic_number = decoded_block_contents[MAGIC_NUMBER_START : MAGIC_NUMBER_START + MAGIC_NUMBER_BYTES_LEN]
        metadata_with_padding = decoded_block_contents[METADATA_START : METADATA_START + METADATA_BYTES_LEN]
        metadata = metadata_with_padding.strip()

        # get number of next block to be used to read
        next_block_with_padding = decoded_block_contents[NEXT_BLOCK_START : NEXT_BLOCK_START + NEXT_BLOCK_BYTES_LEN]
        # next_block = int(next_block_with_padding.strip())
        next_block = int(next_block_with_padding.strip())

        #read_hashed_contents = decoded_block_contents[HASH_START : HASH_START + HASH_BYTES_LEN]
        #hashed_contents = hash_block_contents(bytes([ord(read_magic_number)]) + metadata.encode(ENCODING) + next_block_with_padding.encode(ENCODING))

        #decoded_hashed_contents = hashed_contents.decode(ENCODING)
        
        if magic_number == read_magic_number:
        #if magic_number == read_magic_number and read_hashed_contents == decoded_hashed_contents:
            # Block {} matches!".format(random_physical_block))
            return (metadata, next_block)
        elif metadata_block_num != 0:
            print("---> ERROR: Corrupted block {}".format(random_physical_block))
            sys.exit(os.EX_DATAERR)
        print("XXXXXXXXX")
        break


# -------------------------------------------------------
# ------------------- Testing functions ----------------- 
# -------------------------------------------------------
def cleanup():
    global ciphered_blocks
    global bitmap
    
    ciphered_blocks = {}
    bitmap = ""


def success_test():
    print("************ SUCCESS TEST ************")

    password = "123456789-daniela"
    total_physical_blocks = 957199
    parse_bitmap_file()
    metadata = bitmap
    orig_metadata = metadata
    retrieved_metadata = ""

    key = generate_key_from_password(password)

    split_password = password.split("-")
    seed_val = int(split_password[0])
    seed(seed_val) 

    first_random_block = -1

    needed_blocks = get_num_needed_blocks(len(metadata))
    print("This data is going to require {} blocks".format(needed_blocks))

    # Handle first block
    (next_block, first_random_block) = hide_metadata_block(password, key, metadata[: METADATA_BYTES_LEN], total_physical_blocks, 0, seed, -1, len(metadata) <= METADATA_BYTES_LEN)
    block_index = 1
    metadata = metadata[METADATA_BYTES_LEN :]
    while metadata != "":
        next_block = hide_metadata_block(password, key, metadata[:  METADATA_BYTES_LEN], total_physical_blocks, block_index, seed, next_block, len(metadata) <= METADATA_BYTES_LEN)
        block_index += 1
        metadata = metadata[METADATA_BYTES_LEN :]
    
    print("----- USED BLOCKS: " + str(block_index))
    if (block_index != needed_blocks):
        print(">>> TEST FAILED: Should have encoded {}!".format(needed_blocks))
    else:
        print(">>> TEST SUCCESS: Successfully encoded {}!".format(needed_blocks))

    seed(seed_val) # resets pseudorandom number generator

    block_index = 0
    # do first iteration outside
    ret_value = retrieve_metadata_block(password, key, total_physical_blocks, block_index, seed_val, first_random_block, -1)
    if (ret_value is not None):
        (metadata_part, next_block) = ret_value
        retrieved_metadata = retrieved_metadata + metadata_part
        block_index = 1 # first block has already been processed
    while next_block != -1:
        ret_value = retrieve_metadata_block(password, key, total_physical_blocks, block_index, seed_val, first_random_block, next_block)
        if (ret_value is not None):
            (metadata_part, next_block) = ret_value
            retrieved_metadata = retrieved_metadata + metadata_part
            block_index += 1

    print("----- USED BLOCKS: " + str(block_index))
    if (block_index != needed_blocks):
        print(">>> TEST FAILED: Should have retrieved {}!".format(needed_blocks))
    else: 
        print(">>> TEST SUCCESS: Successfully retrieved {}!".format(needed_blocks))

    if orig_metadata != retrieved_metadata:
        print(">>> TEST FAILED: Did not correctly retrieve metadata!")
    else: 
        print(">>> TEST SUCCESS: Correctly retrieve metadata!")


def first_random_block_occupied_test(random_block_numbers):
    global bitmap
    print("************ FIRST RANDOM BLOCK OCCUPIED TEST ************")

    password = "123456789-daniela"
    total_physical_blocks = 957199
    parse_bitmap_file()
    set_bit_in_bitmap(random_block_numbers[0]) # force first random block to be set
    metadata = bitmap
    orig_metadata = metadata
    retrieved_metadata = ""

    key = generate_key_from_password(password)

    split_password = password.split("-")
    seed_val = int(split_password[0])
    seed(seed_val) 

    first_random_block = -1

    needed_blocks = get_num_needed_blocks(len(metadata))
    print("This data is going to require {} blocks".format(needed_blocks))

    (next_block, first_random_block) = hide_metadata_block(password, key, metadata[: METADATA_BYTES_LEN], total_physical_blocks, 0, seed, -1, len(metadata) <= METADATA_BYTES_LEN)
    block_index = 1
    metadata = metadata[METADATA_BYTES_LEN :]
    while metadata != "":
        next_block = hide_metadata_block(password, key, metadata[:  METADATA_BYTES_LEN], total_physical_blocks, block_index, seed, next_block, len(metadata) <= METADATA_BYTES_LEN)
        block_index += 1
        metadata = metadata[METADATA_BYTES_LEN :]
    
    print("----- USED BLOCKS: " + str(block_index))
    if (block_index != needed_blocks):
        print(">>> TEST FAILED: Should have encoded {}!".format(needed_blocks))
    else:
        print(">>> TEST SUCCESS: Successfully encoded {}!".format(needed_blocks))

    seed(seed_val) # resets pseudorandom number generator

    block_index = 0
    # do first iteration outside
    ret_value = retrieve_metadata_block(password, key, total_physical_blocks, block_index, seed_val, first_random_block, -1)
    if (ret_value is not None):
        (metadata_part, next_block) = ret_value
        retrieved_metadata = retrieved_metadata + metadata_part
        block_index = 1 # first block has already been processed
    while next_block != -1:
        ret_value = retrieve_metadata_block(password, key, total_physical_blocks, block_index, seed_val, first_random_block, next_block)
        if (ret_value is not None):
            (metadata_part, next_block) = ret_value
            retrieved_metadata = retrieved_metadata + metadata_part
            block_index += 1

    print("----- USED BLOCKS: " + str(block_index))
    if (block_index != needed_blocks):
        print(">>> TEST FAILED: Should have retrieved {}!".format(needed_blocks))
    else: 
        print(">>> TEST SUCCESS: Successfully retrieved {}!".format(needed_blocks))

    if orig_metadata != retrieved_metadata:
        print(">>> TEST FAILED: Did not correctly retrieve metadata!")
    else: 
        print(">>> TEST SUCCESS: Correctly retrieve metadata!")


def two_first_random_blocks_occupied_test(random_block_numbers):
    global bitmap
    print("************ TWO FIRST RANDOM BLOCKS OCCUPIED TEST ************")

    password = "123456789-daniela"
    total_physical_blocks = 957199
    parse_bitmap_file()
    set_bit_in_bitmap(random_block_numbers[0]) # force first random block to be set
    set_bit_in_bitmap(random_block_numbers[1]) # force second random block to be set
    metadata = bitmap
    orig_metadata = metadata
    retrieved_metadata = ""

    key = generate_key_from_password(password)

    split_password = password.split("-")
    seed_val = int(split_password[0])
    seed(seed_val) 

    first_random_block = -1

    needed_blocks = get_num_needed_blocks(len(metadata))
    print("This data is going to require {} blocks".format(needed_blocks))

    # Handle first block
    (next_block, first_random_block) = hide_metadata_block(password, key, metadata[: METADATA_BYTES_LEN], total_physical_blocks, 0, seed, -1, len(metadata) <= METADATA_BYTES_LEN)
    block_index = 1
    metadata = metadata[METADATA_BYTES_LEN :]
    while metadata != "":
        next_block = hide_metadata_block(password, key, metadata[:  METADATA_BYTES_LEN], total_physical_blocks, block_index, seed, next_block, len(metadata) <= METADATA_BYTES_LEN)
        block_index += 1
        metadata = metadata[METADATA_BYTES_LEN :]
    
    print("----- USED BLOCKS: " + str(block_index))
    if (block_index != needed_blocks):
        print(">>> TEST FAILED: Should have encoded {}!".format(needed_blocks))
    else:
        print(">>> TEST SUCCESS: Successfully encoded {}!".format(needed_blocks))

    seed(seed_val) # resets pseudorandom number generator

    block_index = 0
    # do first iteration outside
    ret_value = retrieve_metadata_block(password, key, total_physical_blocks, block_index, seed_val, first_random_block, -1)
    if (ret_value is not None):
        (metadata_part, next_block) = ret_value
        retrieved_metadata = retrieved_metadata + metadata_part
        block_index = 1 # first block has already been processed
    while next_block != -1:
        ret_value = retrieve_metadata_block(password, key, total_physical_blocks, block_index, seed_val, first_random_block, next_block)
        if (ret_value is not None):
            (metadata_part, next_block) = ret_value
            retrieved_metadata = retrieved_metadata + metadata_part
            block_index += 1

    print("----- USED BLOCKS: " + str(block_index))
    if (block_index != needed_blocks):
        print(">>> TEST FAILED: Should have retrieved {}!".format(needed_blocks))
    else: 
        print(">>> TEST SUCCESS: Successfully retrieved {}!".format(needed_blocks))

    if orig_metadata != retrieved_metadata:
        print(">>> TEST FAILED: Did not correctly retrieve metadata!")
    else: 
        print(">>> TEST SUCCESS: Correctly retrieve metadata!")


def tampered_block_test(random_block_numbers):
    global bitmap
    print("************ TAMPERED TEST ************")

    password = "123456789-daniela"
    total_physical_blocks = 957199
    parse_bitmap_file()
    metadata = bitmap
    orig_metadata = metadata
    retrieved_metadata = ""

    key = generate_key_from_password(password)

    split_password = password.split("-")
    seed_val = int(split_password[0])
    seed(seed_val) 

    first_random_block = -1

    needed_blocks = get_num_needed_blocks(len(metadata))
    print("This data is going to require {} blocks".format(needed_blocks))

    # Handle first block
    (next_block, first_random_block) = hide_metadata_block(password, key, metadata[: METADATA_BYTES_LEN], total_physical_blocks, 0, seed, -1, len(metadata) <= METADATA_BYTES_LEN)
    block_index = 1
    metadata = metadata[METADATA_BYTES_LEN :]
    while metadata != "":
        next_block = hide_metadata_block(password, key, metadata[:  METADATA_BYTES_LEN], total_physical_blocks, block_index, seed, next_block, len(metadata) <= METADATA_BYTES_LEN)
        block_index += 1
        metadata = metadata[METADATA_BYTES_LEN :]
    
    print("----- USED BLOCKS: " + str(block_index))
    if (block_index != needed_blocks):
        print(">>> TEST FAILED: Should have encoded {}!".format(needed_blocks))
    else:
        print(">>> TEST SUCCESS: Successfully encoded {}!".format(needed_blocks))

    seed(seed_val) # resets pseudorandom number generator

    # Tamper with data on first block
    ciphered_blocks[random_block_numbers[0]] = ciphered_blocks[random_block_numbers[0]][: 1000] + b'A' + ciphered_blocks[random_block_numbers[0]][1001 :]

    block_index = 0
    # do first iteration outside
    ret_value = retrieve_metadata_block(password, key, total_physical_blocks, block_index, seed_val, first_random_block, -1)
    if (ret_value is not None):
        (metadata_part, next_block) = ret_value
        retrieved_metadata = retrieved_metadata + metadata_part
        block_index = 1 # first block has already been processed
    while next_block != -1:
        ret_value = retrieve_metadata_block(password, key, total_physical_blocks, block_index, seed_val, first_random_block, next_block)
        if (ret_value is not None):
            (metadata_part, next_block) = ret_value
            retrieved_metadata = retrieved_metadata + metadata_part
            block_index += 1

    print("----- USED BLOCKS: " + str(block_index))
    # This test will fail here because since our block is tampered with, we
    # cannot obtain the number of the next block
    # TODO: so when we add replication, we should search for the next block
    if (block_index != 0):
        print(">>> TEST FAILED: Should have retrieved 0 blocks!")
    else:
        print(">>> TEST SUCCES: Correctly retrieved 0 block!")

    if retrieved_metadata != "":
        print(">>> TEST FAILED: Did not correctly retrieve none of the metadata!")
    else: 
        print(">>> TEST SUCCESS: Correctly retrieved none of the metadata!")


if __name__ == '__main__':
    total_physical_blocks = 957199
    random_block_numbers = generate_sequence_of_random_block_numbers_with_seed("123456789-daniela", 5, total_physical_blocks)

    # order of 25 blocks used: 896622, 12256-12279
    success_test()
    cleanup()
    first_random_block_occupied_test(random_block_numbers)
    cleanup()
    two_first_random_blocks_occupied_test(random_block_numbers)
    cleanup()
    tampered_block_test(random_block_numbers)