import os
import sys
import pathlib
import math
import numpy as np
from decimal import Decimal
import hashlib

# total blocks in each virtual disk: 262144
# block range: 0 - 262143
BYTES_IN_A_GB = 1073741824

CUR_DIR = str(pathlib.Path(__file__).parent.resolve()) + "/"

PLAINTEXT_FILES_DIRECTORY = CUR_DIR + "../virtual_disks/plaintext/"
IMAGE_FILES_DIRECTORY = CUR_DIR + "../virtual_disks/image/"
COMPRESSED_FILES_DIRECTORY = CUR_DIR + "../virtual_disks/compressed/"
ENCRYPTED_FILES_DIRECTORY = CUR_DIR + "../virtual_disks/encrypted/"

DISKS = ['plaintext', 'image', 'compressed', 'encrypted']
DISKS_FILES_DESCRIPTIONS = ['Plaintext; txt extension', 'JPEG image; jpg extension', 'Zip; zip extension', 'Encryption of a byte sequence; txt extension']
DISKS_DESCRIPTION_FILE = CUR_DIR + "results/disks_description.txt"

COLUMN_NAMES = ['Num', 'Name', 'Size (bytes)', 'md5', 'Description', 'Start block', 'End block']
COLUMN_FRMT_PLAINTEXT = ['r', 'p{6cm}', 'p{1.2cm}', 'p{4cm}', 'p{2cm}', 'R{1.2cm}', 'R{1.2cm}']
COLUMN_FRMT_IMAGE = ['r', 'p{3cm}', 'p{1.2cm}', 'p{5cm}', 'p{4cm}', 'R{1.2cm}', 'R{1.2cm}']
COLUMN_FRMT_COMPRESSED = ['r', 'p{2cm}', 'r', 'l', 'l', 'R{1cm}', 'R{1.2cm}']
COLUMN_FRMT_ENCRYPTED = ['r', 'p{2cm}', 'r', 'l', 'p{2.7cm}', 'R{1cm}', 'R{1.2cm}']
CAPTIONS = ['Table describing initial contents of plaintext virtual disk, taken from the Gutenberg ebook repository \cite{gutenberg-plaintext-datasets}', 'Table describing initial contents of image virtual disk, composed by several JPEG images taken from COCO\'s online object dataset (Common Objects in Context), more specifically the 2017 test images \cite{coco-image-datasets}', 'Table describing initial contents of compressed virtual disk, obtained with the compression of a folder with several JPEG image files', 'Table describing initial contents of encrypted virtual disk, consisting on a file composed of 0\'s with db extension and encrypted using triple DES algorithm from openssl implementation with txt extension']
LABELS = ['tab:plaintext-virtual-disk-contents', 'tab:image-virtual-disk-contents', 'tab:compressed-virtual-disk-contents', 'tab:encrypted-virtual-disk-contents']

LEFT_MARGIN = "-20pt"
RIGHT_MARGIN = "0pt"
TEXT_LINE_SPACE = "6pt"
CELL_TEXT_SPACING = "1"
COL_SPACING = "2pt"
COL_SPACING_SMALL = "3.8pt"

# \begin{center}
# % margins spacing
# \setlength\LTleft{-20pt}
# \setlength\LTright{0pt}
# % spacing between lines
# \setlength\extrarowheight{6pt}
# % spacing between text lines of the same cell
# \begin{spacing}{1}
# % spacing between columns
# \setlength{\tabcolsep}{2pt}
# \begin{longtable}{@{\extracolsep{\fill}}lp{6cm}p{1.2cm}p{4cm}p{2cm}p{1.2cm}p{1.2cm}@{}}

# \begin{longtable}{@{\extracolsep{\fill}}lp{3cm}p{1.2cm}p{5cm}p{4cm}p{1.2cm}p{1.2cm}@{}}

# \begin{table}[!htb]
# 	\centering
# \begin{adjustwidth}{-20pt}{0pt}
# 	\renewcommand{\arraystretch}{1.2}
# 	% spacing between columns
#     \setlength{\tabcolsep}{3.8pt}
# 	\begin{tabular}{@{}rp{2cm}rllR{1cm}R{1.2cm}@{}}
# 		\toprule
# 			Num & Name & Size (bytes) & md5 & Description & Start block & End block \\
# 		\midrule
# 			1 & test2017.zip & 6646970404 & 77ad2c53ac5d0aea611d422c0938fb35 & Zip; zip extension & 0 & 262143 \\
# 		\bottomrule
# 	\end{tabular}
# 	\label{tab:compressed-virtual-disk-contents}
# \end{adjustwidth}
# \caption{Table describing initial contents of compressed virtual disk, obtained with the compression of a folder with several JPEG image files}
# \end{table}

def escape_latex(s):
    new_s = []
    for c in s:
        if c in '#$%&_{}':
            new_s.extend('\\'+c)
        elif c == '\\':
            new_s.extend('\\textbackslash{}')
        elif c == '^':
            new_s.extend('\\textasciicircum{}')
        elif c == '~':
            new_s.extend('\\textasciitilde{}')
        else:
            new_s.append(c)
    return ''.join(new_s)


def get_file_size(file):
    file.seek(0, os.SEEK_END)
    file_size = file.tell()
    # set back to beginning
    file.seek(0)
    return file_size


def hash_md5_file(file_complete_path):
    hash_md5 = hashlib.md5()
    with open(file_complete_path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()


def write_latex_table_header(results_file, left_margin, right_margin, column_frmts, column_names):
    results_file.write("\\begin{table}[!htb]\n")
    results_file.write("\t\\centering\n")
    results_file.write("\\begin{adjustwidth}{" + left_margin + "}{" + right_margin + "}\n")
    results_file.write("\t\\renewcommand{\\arraystretch}{1.2}\n")
    results_file.write("% spacing between columns\n")
    results_file.write("\\setlength{\\tabcolsep}{" + COL_SPACING_SMALL + "}\n")
    results_file.write("\t\\begin{tabular}{@{}")
    for frmt in column_frmts:
        results_file.write(frmt)
    results_file.write("@{}}\n")
    results_file.write("\t\t\\toprule\n")

    results_file.write("\t\t\t")
    num_cols = len(column_names)
    for i in range(0, num_cols - 1):
        results_file.write("{} & ".format(column_names[i]))
    results_file.write("{} ".format(column_names[num_cols - 1]))
    results_file.write("\\\\\n")

    results_file.write("\t\t\\midrule\n")


def write_latex_table_footer(results_file, caption, label):
    results_file.write("\t\t\\bottomrule\n")
    results_file.write("\t\\end{tabular}\n")
    results_file.write("\t\\label{}\n".format("{" + label + "}"))
    results_file.write("\\end{adjustwidth}\n")
    results_file.write("\t\\caption{}\n".format("{" + caption + "}"))
    results_file.write("\\end{table}\n")

    results_file.write("\n\n")


def write_latex_table_content_line(results_file, line_contents):
    results_file.write("\t\t\t")
    num_cells = len(line_contents)
    for i in range(0, num_cells - 1):
        results_file.write("{} & ".format(line_contents[i]))
    results_file.write("{} ".format(line_contents[num_cells - 1]))
    results_file.write("\\\\\n")


def write_latex_longtable_header(results_file, left_margin, right_margin, line_spacing, cell_text_spacing, col_spacing, caption, column_frmts, column_names):
    results_file.write("\\begin{center}\n")
    results_file.write("\\captionsetup{width=\\textwidth}\n")
    results_file.write("% margins spacing\n")
    results_file.write("\\setlength\\LTleft{}\n".format("{" + left_margin + "}"))
    results_file.write("\\setlength\\LTright{}\n".format("{" + right_margin + "}"))
    results_file.write("% spacing between lines\n")
    results_file.write("\\setlength\\extrarowheight{}\n".format("{" + line_spacing + "}"))
    results_file.write("% spacing between text lines of the same cell\n")
    results_file.write("\\begin{spacing}{" + cell_text_spacing + "}\n")
    results_file.write("% spacing between columns\n")
    results_file.write("\\setlength{\\tabcolsep}{" + col_spacing + "}\n")
    results_file.write("\\begin{longtable}{@{\\extracolsep{\\fill}}")
    for frmt in column_frmts:
        results_file.write(frmt)
    results_file.write("@{}}\n")
    results_file.write("\t\\caption{}\n".format("{" + caption + "}\\\\"))
    results_file.write("\t\t\\toprule\n")

    results_file.write("\t\t\t")
    num_cols = len(column_names)
    for i in range(0, num_cols - 1):
        results_file.write("{} & ".format(column_names[i]))
    results_file.write("{} ".format(column_names[num_cols - 1]))
    results_file.write("\\\\\n")

    results_file.write("\t\t\\midrule\n")


def write_latex_longtable_footer(results_file, label):
    results_file.write("\t\t\\bottomrule\n")
    results_file.write("\t\\label{}\n".format("{" + label + "}"))
    results_file.write("\\end{longtable}\n")
    results_file.write("\\end{spacing}\n")
    results_file.write("\\end{center}\n")

    results_file.write("\n\n")


def export_disk_files_details_to_latex_table():
    with open(DISKS_DESCRIPTION_FILE, "w") as results_file:
        # clears file
        results_file.truncate(0)

        disk_counter = 0
        
        res = write_longtable(results_file, PLAINTEXT_FILES_DIRECTORY, disk_counter, COLUMN_FRMT_PLAINTEXT)
        disk_counter += 1

        res = write_longtable(results_file, IMAGE_FILES_DIRECTORY, disk_counter, COLUMN_FRMT_IMAGE)
        disk_counter += 1

        res = write_table(results_file, COMPRESSED_FILES_DIRECTORY, disk_counter, COLUMN_FRMT_COMPRESSED)
        disk_counter += 1

        res = write_table(results_file, ENCRYPTED_FILES_DIRECTORY, disk_counter, COLUMN_FRMT_ENCRYPTED)
        disk_counter += 1


def write_longtable(results_file, disk_files_path, disk_counter, column_frmt):
    disk_offset_bytes = 0
    file_counter = 0
    start_block = None 
    end_block = None

    write_latex_longtable_header(results_file, LEFT_MARGIN, RIGHT_MARGIN, TEXT_LINE_SPACE, CELL_TEXT_SPACING, COL_SPACING, CAPTIONS[disk_counter], column_frmt, COLUMN_NAMES)
    print("-> " + disk_files_path + '\n')
    for file_name in os.listdir(disk_files_path):
        # if file_counter > 10:
        #     break
        file_complete_path = os.path.join(disk_files_path, file_name)
        # print("* " + file_complete_path + '\n')
        if not file_name.startswith('.') and os.path.isfile(file_complete_path):
            # print('valid file\n')
            file_counter += 1

            line_contents = []
            line_contents.append(file_counter)
            line_contents.append(escape_latex(file_name))

            with open(file_complete_path, "rb") as file:
                file_size = get_file_size(file)

            line_contents.append(file_size)
            line_contents.append("\seqsplit{" + hash_md5_file(file_complete_path) + "}")
            line_contents.append(DISKS_FILES_DESCRIPTIONS[disk_counter])

            if (disk_offset_bytes + file_size > BYTES_IN_A_GB):
                file_size = BYTES_IN_A_GB - disk_offset_bytes
                start_block = math.ceil(disk_offset_bytes / 4096)
                if (disk_offset_bytes + file_size) % 4096 == 0:
                    end_block = math.ceil((disk_offset_bytes + file_size) / 4096) - 1
                else: 
                    end_block = math.ceil((disk_offset_bytes + file_size) / 4096)

                line_contents.append(str(start_block))
                line_contents.append(str(end_block))

                # print("---- LAST BLOCK ---- ")
                # print("disk_offset_bytes: " + str(disk_offset_bytes))
                # print("amount of blocks left to write: " + str(file_size))
                # print("start block: " + str(start_block))
                # print("end block: " + str(end_block))
                write_latex_table_content_line(results_file, line_contents)
                break
            elif (disk_offset_bytes + file_size == BYTES_IN_A_GB):
                break

            start_block = math.ceil(disk_offset_bytes / 4096)
            if (disk_offset_bytes + file_size) % 4096 == 0:
                end_block = math.ceil((disk_offset_bytes + file_size) / 4096) - 1
            else: 
                end_block = math.ceil((disk_offset_bytes + file_size) / 4096)

            line_contents.append(str(start_block))
            line_contents.append(str(end_block))

            write_latex_table_content_line(results_file, line_contents)

            disk_offset_bytes += file_size

    write_latex_longtable_footer(results_file, LABELS[disk_counter])


def write_table(results_file, disk_files_path, disk_counter, column_frmts):
    disk_offset_bytes = 0
    file_counter = 0
    start_block = None 
    end_block = None

    write_latex_table_header(results_file, LEFT_MARGIN, RIGHT_MARGIN, column_frmts, COLUMN_NAMES)
    print("-> " + disk_files_path + '\n')
    for file_name in os.listdir(disk_files_path):
        # if file_counter > 10:
        #     break
        file_complete_path = os.path.join(disk_files_path, file_name)
        # print("* " + file_complete_path + '\n')
        if not file_name.startswith('.') and os.path.isfile(file_complete_path):
            # print('valid file\n')
            file_counter += 1

            line_contents = []
            line_contents.append(file_counter)
            line_contents.append(escape_latex(file_name))

            with open(file_complete_path, "rb") as file:
                file_size = get_file_size(file)

            line_contents.append(file_size)
            line_contents.append(hash_md5_file(file_complete_path))
            line_contents.append(DISKS_FILES_DESCRIPTIONS[disk_counter])

            if (disk_offset_bytes + file_size > BYTES_IN_A_GB):
                file_size = BYTES_IN_A_GB - disk_offset_bytes
                start_block = math.ceil(disk_offset_bytes / 4096)
                if (disk_offset_bytes + file_size) % 4096 == 0:
                    end_block = math.ceil((disk_offset_bytes + file_size) / 4096) - 1
                else: 
                    end_block = math.ceil((disk_offset_bytes + file_size) / 4096)

                line_contents.append(str(start_block))
                line_contents.append(str(end_block))

                # print("---------- write_latex_table_content_line\n")
                write_latex_table_content_line(results_file, line_contents)
                break
            elif (disk_offset_bytes + file_size == BYTES_IN_A_GB):
                break

            start_block = math.ceil(disk_offset_bytes / 4096)
            if (disk_offset_bytes + file_size) % 4096 == 0:
                end_block = math.ceil((disk_offset_bytes + file_size) / 4096) - 1
            else: 
                end_block = math.ceil((disk_offset_bytes + file_size) / 4096)

            line_contents.append(str(start_block))
            line_contents.append(str(end_block))

            print("---------- write_latex_table_content_line\n")
            write_latex_table_content_line(results_file, line_contents)

            disk_offset_bytes += file_size

    write_latex_table_footer(results_file, CAPTIONS[disk_counter], LABELS[disk_counter])


# to keep in user environment where the links to the libraries are
# $ sudo -E python3 write_virtual_disks_description.py
if __name__ == '__main__':

    # to get info to make the tables describing the virtual disks
    export_disk_files_details_to_latex_table()

