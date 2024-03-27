import re


# set up regular expressions
dumpe2fs_free_blocks_pattern = re.compile(r'(?P<start>\d+)-?(?P<end>\d+)?')

dumpe2fs_dict = {
    'group': re.compile(r'Group (?P<group>\d+):.*\n'),
    'free_blocks': re.compile(r'Free blocks: \d+-?(\d+)?.*\n'),
}


calypso_free_blocks_pattern = re.compile(r'#(?P<start>\d+) to block #(?P<end>\d+)')

calypso_dict = {
    #'group': re.compile(r'.*------ START GROUP # (?P<group>\d+) ------\n'),
    #'group': re.compile(r'\[ \d+.\d+\] calypso: \[calypso_iterate_partition_blocks_groups\] ------ START GROUP # (?P<group>\d+).*\n'),
    'group': re.compile(r'\[ ?\d+.\d+ ?\] calypso: \[calypso_iterate_partition_blocks_groups\] ------ START GROUP # (?P<group>\d+) ------\n'),
    'free_blocks': re.compile(r'\[ ?\d+.\d+ ?\] Empty blocks from block #\d+ to block #\d+\n'),
}


def _parse_line(line, pattern_dict):
    """
    Do a regex search against all defined regexes and
    return the key and match result of the first matching regex

    """

    for key, pattern in pattern_dict.items():
        match = pattern.search(line)
        if match:
            return key, match
    # if there are no matches
    return None, None

def parse_file(filepath, free_blocks_pattern, pattern_dict):

    data = []  # create an empty list to collect the data

    group = 0
    free_blocks_ranges = []
    start = 0
    end = 0

    # open the file and read through it line by line
    with open(filepath, 'r') as file_object:
        line = file_object.readline()
        while line:
            # at each line check for a match with a regex
            key, match = _parse_line(line, pattern_dict)
            if key == 'group':
                group = match.group('group')
                group = int(group)

            if key == 'free_blocks':
                free_blocks_ranges = re.findall(free_blocks_pattern, match.group(0))
                for i in free_blocks_ranges:
                    start = i[0]
                    end = i[1]
                    start = int(start)
                    if end != '':
                        end = int(end)
                    else:
                        end = start

                    row = {
                        'group': group,
                        'start': start,
                        'end': end
                    }
                    data.append(row)

            line = file_object.readline()

        return data


# Note: if a block range is different, all the next ones are also going to be
def cmp_free_blocks(dumpe2fs_free_blocks, calypso_free_blocks):
    equal_free_blocks = True
    for entry1, entry2 in zip(dumpe2fs_free_blocks, calypso_free_blocks):
        if not(entry1['group'] == entry2['group'] and entry1['start'] == entry2['start'] and entry1['end'] == entry2['end']):
            equal_free_blocks = False
            print("Free blocks don't match on group")
    if (equal_free_blocks == True):
        print("Free block ranges are the same")


if __name__ == '__main__':
    #filepath1 = 'sda8_dumpe2fs.txt'
    #filepath2 = 'sda8_calypso_dump.txt'
    #filepath1 = 'sda5_dumpe2fs.txt'
    #filepath2 = 'sda5_calypso_dump.txt'
    filepath1 = '../../calypso_driver/dump_dump2fs.txt'
    filepath2 = '../../calypso_driver/dump_calypso.txt'

    dumpe2fs_free_blocks = parse_file(filepath1, dumpe2fs_free_blocks_pattern, dumpe2fs_dict)
    calypso_free_blocks = parse_file(filepath2, calypso_free_blocks_pattern, calypso_dict)

    print("Dumpe2fs found " + str(len(dumpe2fs_free_blocks)) + " block ranges")
    print("Calypso found " + str(len(calypso_free_blocks)) + " block ranges")

    cmp_free_blocks(dumpe2fs_free_blocks, calypso_free_blocks)