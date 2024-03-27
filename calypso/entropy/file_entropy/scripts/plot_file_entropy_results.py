import os
import sys
import pathlib
import math
import matplotlib.pyplot as plt
import numpy as np
from decimal import Decimal
import random
import seaborn as sns


# TOTAL_VIRTUAL_DISKS = 5
TOTAL_VIRTUAL_DISKS = 4

BYTES_IN_A_GB = 1073741824

CUR_DIR = str(pathlib.Path(__file__).parent.resolve()) + "/"

#
PLAINTEXT_FILES_DIRECTORY = CUR_DIR + "../virtual_disks/plaintext/"
PLAINTEXT_DISK_PATH = "/dev/sdb"

# http://images.cocodataset.org/zips/test2017.zip
IMAGE_FILES_DIRECTORY = CUR_DIR + "../virtual_disks/image/"
IMAGE_DISK_PATH = "/dev/sdc"

#
# VIDEO_FILES_DIRECTORY = CUR_DIR + "../virtual_disks/video/"
# VIDEO_DISK_PATH = "/dev/sdd"

# Compressed http://images.cocodataset.org/zips/test2017.zip
COMPRESSED_FILES_DIRECTORY = CUR_DIR + "../virtual_disks/compressed/"
# COMPRESSED_DISK_PATH = "/dev/sde"
COMPRESSED_DISK_PATH = "/dev/sdd"

#
ENCRYPTED_FILES_DIRECTORY = CUR_DIR + "../virtual_disks/encrypted/"
# ENCRYPTED_DISK_PATH = "/dev/sdf"
ENCRYPTED_DISK_PATH = "/dev/sde"

DISK_PATH = "/dev/sdb"

# file_types_x = ["plaintext", "image", "video", "compressed", "encrypted"]
file_types_x = ["Plaintext", "Image", "Compressed", "Encrypted"]
values_x = []
entropy_values_y = []


LEFT_MARGIN = "0pt"
RIGHT_MARGIN = "0pt"
TEXT_LINE_SPACE = "6pt"
CELL_TEXT_SPACING = "1"
COL_SPACING = "2pt"
COL_SPACING_SMALL = "3.8pt"

AXIS_LABEL_DISTANCE = 15

X_AXIS_LABEL_FONT_SIZE = 50
Y_AXIS_LABEL_FONT_SIZE = 50
X_TICK_LABEL_FONT_SIZE = 46
Y_TICK_LABEL_FONT_SIZE = 46


def plot_bar_file_entropies(data):
    fig, ax = plt.subplots(figsize=(30, 15))

    colors = ['tab:blue', 'tab:orange', 'tab:green', 'tab:pink']
    error_values = []
    averages = [] # y axis
    x_axis = ['Plaintext', 'Images', 'Compressed', 'Encrypted']
    i = 0
    for entropies in data:
        np_array = np.array(entropies)
        averages.append(np.round(np.average(np_array), 2))
        error_values.append(np.std(entropies))
        ax.bar([x_axis[i]], [averages[i]], yerr=[error_values[i]], align='center', alpha=0.7, ecolor='black', capsize=10, color=[colors[i]])
        i += 1

    plt.xlabel("File types", labelpad=AXIS_LABEL_DISTANCE)
    plt.ylabel("Block entropy", labelpad=AXIS_LABEL_DISTANCE)

    fig.set_size_inches(30, 15)
    ax.tick_params(axis='x', which='major', labelsize=X_TICK_LABEL_FONT_SIZE)
    ax.tick_params(axis='y', which='major', labelsize=Y_TICK_LABEL_FONT_SIZE)
    ax.xaxis.label.set_size(X_AXIS_LABEL_FONT_SIZE)
    ax.yaxis.label.set_size(Y_AXIS_LABEL_FONT_SIZE)

    plt.savefig('file_entropies_bars.pdf') 


def plot_file_entropies(results_file):
    blocks_in_a_gb = int(BYTES_IN_A_GB / 4096)
    data = []
    block = 0
    with open(results_file, "r") as results_file: 
        results_content = results_file.readlines()

        for disk in range(0, TOTAL_VIRTUAL_DISKS):
            results_disk = results_content[disk * blocks_in_a_gb + disk + 1 : ]
            data.append([])
            for block in range(0, blocks_in_a_gb):
                parsed_entropy = results_disk[block].split('\n')[0]
                entropy = float(parsed_entropy)
                data[disk].append(entropy)

    fig, ax = plt.subplots()

    # bp = plt.boxplot(data, whis=[5, 95], showfliers=False)
    # sc = plt.scatter([1, 1], [data.min(), data.max()])
    
    # Show only min and max fliers
    bp = ax.boxplot(data, showfliers=False)
    for i in range(0, len(data)):
        y_array = [min(data[i]), max(data[i])]
        x_array = [i + 1, i + 1]
        print("x_array: " + str(x_array))
        print("y_array: " + str(y_array))
        sc = plt.scatter(x_array, y_array)
    for j in data[3]:
        if j == 0:
            print("FOUND 0!")

    # rename x values
    plt.xticks(range(1, TOTAL_VIRTUAL_DISKS + 1), file_types_x)
    plt.xlabel('Virtual disk file type')
    plt.ylabel('Block entropy')
    plt.savefig('file_entropies_plot2.pdf') 

    table_file_entropies(data) 
    plot_bar_file_entropies(data)


def write_latex_table_header(results_file, left_margin, right_margin, column_frmts, column_names):
    results_file.write("\\begin{table}[!htb]\n")
    results_file.write("\t\\centering\n")
    results_file.write("\\begin{adjustwidth}{" + left_margin + "}{" + right_margin + "}\n")
    results_file.write("\t\\renewcommand{\\arraystretch}{1.2}\n")
    results_file.write("% spacing between columns\n")
    results_file.write("\\setlength{\\tabcolsep}{" + COL_SPACING + "}\n")
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


def table_file_entropies(data):
    error_values = []
    for entropies in data:
        error_values.append(np.std(entropies))

    with open("entropy_overview_statistics.txt", "w") as results_file:
        column_frmt = ['p{2.5cm}', 'R{2.1cm}', 'R{2.1cm}', 'R{2.1cm}', 'R{2.1cm}', 'R{2.1cm}', 'R{2.1cm}']
        title_cols = ['Type of files', 'Average $\\pm$ error', 'Minimum', '1st Quartile', 'Mediane', '3rd Quartile', 'Maximum']
        write_latex_table_header(results_file, LEFT_MARGIN, RIGHT_MARGIN, column_frmt, title_cols)
        results_file.write("\t\t\\midrule\n")

        line_contents = []
        np_array = np.array(data[0])
        line_contents.append('Plaintext')
        line_contents.append("{:.4f} ".format(np.round(np.average(np_array), 4)) + "$\\pm$" + " {:.4f}".format(error_values[0]))
        line_contents.append("{:.4f}".format(np.round(np_array.min(), 4)))
        line_contents.append("{:.4f}".format(np.round(np.quantile(np_array, 0.25), 4)))
        line_contents.append("{:.4f}".format(np.round(np.median(np_array), 4)))
        line_contents.append("{:.4f}".format(np.round(np.quantile(np_array, 0.75), 4)))
        line_contents.append("{:.4f}".format(np.round(np_array.max(), 4)))
        write_latex_table_content_line(results_file, line_contents)
        results_file.write("\t\t\\midrule\n")

        line_contents = []
        np_array = np.array(data[1])
        line_contents.append('Image')
        line_contents.append("{:.4f} ".format(np.round(np.average(np_array), 4)) + "$\\pm$" + " {:.4f}".format(error_values[1]))
        line_contents.append("{:.4f}".format(np.round(np_array.min(), 4)))
        line_contents.append("{:.4f}".format(np.round(np.quantile(np_array, 0.25), 4)))
        line_contents.append("{:.4f}".format(np.round(np.median(np_array), 4)))
        line_contents.append("{:.4f}".format(np.round(np.quantile(np_array, 0.75), 4)))
        line_contents.append("{:.4f}".format(np.round(np_array.max(), 4)))
        write_latex_table_content_line(results_file, line_contents)
        results_file.write("\t\t\\midrule\n")

        line_contents = []
        np_array = np.array(data[2])
        line_contents.append('Compressed')
        line_contents.append("{:.4f} ".format(np.round(np.average(np_array), 4)) + "$\\pm$" + " {:.4f}".format(error_values[2]))
        line_contents.append("{:.4f}".format(np.round(np_array.min(), 4)))
        line_contents.append("{:.4f}".format(np.round(np.quantile(np_array, 0.25), 4)))
        line_contents.append("{:.4f}".format(np.round(np.median(np_array), 4)))
        line_contents.append("{:.4f}".format(np.round(np.quantile(np_array, 0.75), 4)))
        line_contents.append("{:.4f}".format(np.round(np_array.max(), 4)))
        write_latex_table_content_line(results_file, line_contents)
        results_file.write("\t\t\\midrule\n")

        line_contents = []
        np_array = np.array(data[3])
        line_contents.append('Encrypted')
        line_contents.append("{:.4f} ".format(np.round(np.average(np_array), 4)) + "$\\pm$" + " {:.4f}".format(error_values[3]))
        line_contents.append("{:.4f}".format(np.round(np_array.min(), 4)))
        line_contents.append("{:.4f}".format(np.round(np.quantile(np_array, 0.25), 4)))
        line_contents.append("{:.4f}".format(np.round(np.median(np_array), 4)))
        line_contents.append("{:.4f}".format(np.round(np.quantile(np_array, 0.75), 4)))
        line_contents.append("{:.4f}".format(np.round(np_array.max(), 4)))
        write_latex_table_content_line(results_file, line_contents)

        write_latex_table_footer(results_file, 'Statistics on the entropy variation in the blocks of 1GB virtual disks simulating unused space, for multiple media file types', 'tab:entropy_overview_statistics')


# $ python3 plot_file_entropy_results.py "results/entropy_results_baseline4.txt"
if __name__ == '__main__':
    # [1] because the first argument is the name of this python file
    results_file = CUR_DIR + sys.argv[1]
    # print("sys.argv[0] " + str(sys.argv))
    # print("results_file " + str(results_file))
    plot_file_entropies(results_file)