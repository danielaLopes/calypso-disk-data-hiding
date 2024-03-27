import pathlib
import math
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

from numpy.ma import masked_array


TOTAL_VIRTUAL_DISKS = 4

BYTES_IN_A_GB = 1073741824
blocks_in_a_gb = int(BYTES_IN_A_GB / 4096)
total_blocks_in_virtual_disks = blocks_in_a_gb * TOTAL_VIRTUAL_DISKS

CUR_DIR = str(pathlib.Path(__file__).parent.resolve()) + "/"

RESULTS_FILE_BEFORE = "results/entropy_results_baseline4.txt"
RESULTS_FILES_AFTER = ["results/entropy_results_after_threshold_0.txt", \
    "results/entropy_results_after_threshold_1.txt", \
    "results/entropy_results_after_threshold_2.txt", \
    "results/entropy_results_after_threshold_3.txt", \
    "results/entropy_results_after_threshold_4.txt", \
    "results/entropy_results_after_threshold_5.txt", \
    "results/entropy_results_after_threshold_6.txt", \
    "results/entropy_results_after_threshold_7.txt"]

PLOT_FILE_NAME = "entropy_differential_plot.pdf"


LEFT_MARGIN = "0pt"
RIGHT_MARGIN = "0pt"
TEXT_LINE_SPACE = "6pt"
CELL_TEXT_SPACING = "1"
COL_SPACING = "2pt"
COL_SPACING_SMALL = "3.8pt"


# COLORS = ['white', 'bisque', 'lightsalmon', 'tomato', 'crimson', 'maroon']
# COLOR_MAP = plt.cm.get_cmap('rainbow', 5)
COLOR_MAP = plt.cm.get_cmap('Oranges')
COLOR_MAP2 = plt.cm.get_cmap('Blues')
# COLOR_MAP = matplotlib.colors.ListedColormap(COLORS)


LEGEND_LOC = "upper left"
LEGEND_FONT_SIZE = 6

X_AXIS_LABEL_FONT_SIZE = 15
Y_AXIS_LABEL_FONT_SIZE = 15
X_TICK_LABEL_FONT_SIZE = 13
Y_TICK_LABEL_FONT_SIZE = 13


def highlight_cell(x,y, ax=None, **kwargs):
    rect = plt.Rectangle((x-.415, y-.39), 0.8, 0.8, fill=False, **kwargs)
    ax = ax or plt.gca()
    ax.add_patch(rect)
    return rect


def waffle_chart_diffs2(data, before_results_file_path, after_results_file_path):
    n_rows = 10 # max entropy diff or average entropy diff
    n_cols = 8 # entropy threshold

    # waffle = np.zeros((n_rows + 1, n_cols))
    waffle = np.zeros((n_rows, n_cols))

    fig, ax = plt.subplots(figsize=(100, 100))

    max_entropy_diff = 1
    step = 0.1
    buckets = np.arange(0, max_entropy_diff + step, step)
    pdfs = []
    for i in range(0, n_cols):
        count, bins_count = np.histogram(data[i], bins=buckets)
        pdf = count / sum(count)
        pdfs.append(pdf)
    print("pdfs: " + str(pdfs))

    i = 0
    # value in each rectangle of the waffle is going to correspond to the storage capacity
    for col in range(n_cols):
        for row in range(n_rows):
            if pdfs[col][row] == 0:
                waffle[row, col] = -0.25
            else:
                waffle[row, col] = pdfs[col][row]
            i += 1
        # waffle[n_rows, col] = -count_modified_blocks(before_results_file_path, after_results_file_path[col], col) / total_blocks_in_virtual_disks 

    fig = plt.figure()

    v1a = masked_array(waffle,waffle<-0.25)
    v1b = masked_array(waffle,waffle>=-0.25)
    # pa = plt.matshow(waffle, cmap=COLOR_MAP, origin='lower')
    # pb = plt.matshow(waffle2, cmap=COLOR_MAP2, origin='lower')
    pa = plt.imshow(v1a, cmap=COLOR_MAP, interpolation='nearest', origin='lower')
    # pb = plt.imshow(v1b, cmap=COLOR_MAP2, interpolation='nearest', origin='lower')

    # borders
    for col in range(n_cols):
        # for row  in range(n_rows + 1):
        for row  in range(n_rows):
            highlight_cell(col, row, color="black", linewidth=1)
    
    ax = plt.gca()
    ax.set_xticks(np.arange(-0.5, (n_cols), 1), minor=True)
    ax.set_yticks(np.arange(-0.5, (n_rows), 1), minor=True)

    ax.tick_params(axis="x", bottom=True, top=False, labelbottom=True, labeltop=False)
    ax.tick_params(axis='x', which='major', labelsize=X_TICK_LABEL_FONT_SIZE)
    ax.tick_params(axis='y', which='major', labelsize=Y_TICK_LABEL_FONT_SIZE)
    ax.xaxis.label.set_size(X_AXIS_LABEL_FONT_SIZE)
    ax.yaxis.label.set_size(Y_AXIS_LABEL_FONT_SIZE)

    ax.grid(which='minor', color='w', linestyle='-', linewidth=4)
    plt.xticks(np.arange(0, n_cols, 1)) # entropy threshold
    ax.set_xticklabels(['0', '1', '2', '3', '4', '5', '6', '7'])
    # plt.yticks(np.arange(0, n_rows + 1, 1)) # entropy differential
    # ax.set_yticklabels(['0.1', '0.2', '0.3', '0.4', '0.5', '0.6', '0.7', '0.8', '0.9', '1.0', 'Storage capacity'])
    plt.yticks(np.arange(0, n_rows, 1)) # entropy differential
    ax.set_yticklabels(['0.1', '0.2', '0.3', '0.4', '0.5', '0.6', '0.7', '0.8', '0.9', '1.0'])

    # norm = matplotlib.colors.Normalize(vmin=0.0, vmax=5)
    # categories = ['0 >= pdf > 0.25', '0.25 >= pdf > 0.5', '0.5 >= pdf > 0.75', '0.75 >= pdf > 1']
    # legend_handles = []
    # for i, category in enumerate(categories):
    #     label_str = category
    #     legend_handles.append(mpatches.Patch(color=COLOR_MAP(norm(i)), label=label_str))
    # plt.legend(handles=legend_handles, loc=LEGEND_LOC, ncol=2,
    # bbox_to_anchor=(0, 1, 0, 0), # positioning legends
    # prop={"size":LEGEND_FONT_SIZE}) # font size

    cbar1 = plt.colorbar(pa, shrink=1)
    # cbar2 = plt.colorbar(pb, shrink=0.75)
    # cbar = plt.colorbar(label='Probability density function (PDF)', rotation=270)
    ticklabels = cbar1.ax.get_ymajorticklabels()
    minVal = 0
    maxVal = np.max(waffle[np.nonzero(waffle)])
    cbar1.set_ticks([minVal, maxVal])
    cbar1.set_ticklabels([0, 1])
    # cbar.ax.get_yaxis().set_ticks([])
    # cbar.set_ticks([0])
    # cbar.set_ticklabels(["0 >= pdf > 0.25"])
    # for j, lab in enumerate(['1','2','3','4']):
        # cbar.ax.text(.5, (2 * j + 1) / 8.0, lab)
    cbar1.set_label('Probability density function (PDF)', labelpad=3, size=X_AXIS_LABEL_FONT_SIZE)
    cbar1.ax.tick_params(labelsize=X_TICK_LABEL_FONT_SIZE) 

    plt.xlabel('Entropy threshold')
    plt.ylabel('Entropy differential')

    # plt.show()
    plt.savefig("waffle_chart_diffs.pdf")


def waffle_chart_diffs(data):
    n_rows = 11 # max entropy diff or average entropy diff
    n_cols = 8 # entropy threshold

    waffle = np.zeros(n_rows * n_cols)

    max_entropy_diff = 1
    step = 0.1
    buckets = np.arange(0, max_entropy_diff + step + step, step)

    fig, ax = plt.subplots(figsize=(30, 15))

    print("buckets: " + str(buckets))
    print("data: " + str(data))

    pdfs = []
    for i in range(0, n_cols):
        count, bins_count = np.histogram(data[i], bins=buckets)
        print("bins_count: " +  str(bins_count))
        pdf = count / sum(count)
        pdfs.append(pdf)
    print("pdfs: " + str(pdfs))

    i = 0
    # value in each rectangle of the waffle is going to correspond to the storage capacity
    for col in range(n_cols):
        for row in range(n_rows):
            waffle[i] = pdfs[col][row]
            i += 1

    waffle_matrix = waffle.reshape(n_rows, n_cols)

    fig = plt.figure()
    colormap = plt.cm.coolwarm
    plt.matshow(waffle_matrix, cmap=colormap, origin='lower')
    ax = plt.gca()
    ax.set_xticks(np.arange(-0.5, (n_cols), 1), minor=True)
    ax.set_yticks(np.arange(-0.5, (n_rows), 1), minor=True)
    ax.tick_params(axis="x", bottom=True, top=False, labelbottom=True, labeltop=False)
    ax.grid(which='minor', color='w', linestyle='-', linewidth=4)
    plt.xticks([])
    plt.yticks([])
    plt.xticks(np.arange(0, n_cols, 1)) # entropy threshold
    ax.set_xticklabels(['0', '1', '2', '3', '4', '5', '6', '7'])
    plt.yticks(np.arange(0, n_rows, 1)) # entropy differential
    ax.set_yticklabels(['0.0', '0.1', '0.2', '0.3', '0.4', '0.5', '0.6', '0.7', '0.8', '0.9', '1.0'])

    plt.colorbar()

    plt.show()


def plot_cumulative_entropy(block_entropies):
    max_entropy_diff = 8
    step = 0.05
    buckets = np.arange(0, max_entropy_diff + step, step)

    fig, ax = plt.subplots(figsize=(30, 15))

    count, bins_count = np.histogram(block_entropies, bins=buckets)
    pdf = count / sum(count)
    cdf = np.cumsum(pdf)

    ax.plot(bins_count[1:], cdf)

    plt.xlabel('Block entropy')
    plt.ylabel('Probability')

    ax.tick_params(axis='both', which='major', labelsize=20)
    fig.set_size_inches(30, 15)
    ax.xaxis.label.set_size(24)
    ax.yaxis.label.set_size(24)
    
    plt.savefig("cumulative_entropy.pdf") 


def plot_probability_entropy_similarity_by_threshold(block_entropies_similarity, threshold):
    max_entropy_diff = 1
    step=0.05
    buckets = np.arange(0, max_entropy_diff + step, step)

    fig, ax = plt.subplots(figsize=(30, 30))

    count, bins_count = np.histogram(block_entropies_similarity[threshold], bins=buckets)
    pdf = count / sum(count)
    ax.plot(bins_count[1:], pdf, '-o', alpha=0.5, label="Threshold = {}".format(threshold))

    plt.xlabel('Entropy similarity')
    plt.ylabel('Probability')

    ax.tick_params(axis='both', which='major', labelsize=20)
    fig.set_size_inches(30, 15)
    ax.xaxis.label.set_size(24)
    ax.yaxis.label.set_size(24)
    
    plt.legend()
    plt.savefig("probability_entropy_similarity{}.pdf".format(threshold)) 


def plot_probability_entropy_similarity(block_entropies_similarity):
    max_entropy_diff = 1
    step=0.05
    buckets = np.arange(0, max_entropy_diff + step, step)

    fig, ax = plt.subplots(figsize=(30, 30))

    for i in range(0, 8):
        count, bins_count = np.histogram(block_entropies_similarity[i], bins=buckets)
        pdf = count / sum(count)
        ax.plot(bins_count[1:], pdf, '-o', alpha=0.5, label="Threshold = {}".format(i))

    plt.xlabel('Entropy similarity')
    plt.ylabel('Probability')

    ax.tick_params(axis='both', which='major', labelsize=20)
    fig.set_size_inches(30, 15)
    ax.xaxis.label.set_size(24)
    ax.yaxis.label.set_size(24)
    
    plt.legend()
    plt.savefig("probability_entropy_similarity.pdf") 


def plot_probability_entropy_similarity_grouped(block_entropies_similarity):
    max_entropy_diff = 1
    step=0.05
    buckets = np.arange(0, max_entropy_diff + step, step)

    fig, ax = plt.subplots(figsize=(30, 30))

    for i in [0, 4, 7]:
        count, bins_count = np.histogram(block_entropies_similarity[i], bins=buckets)
        pdf = count / sum(count)
        ax.plot(bins_count[1:], pdf, '-o', alpha=0.5, label="Threshold = {}".format(i))

    plt.xlabel('Entropy similarity')
    plt.ylabel('Probability')

    ax.tick_params(axis='both', which='major', labelsize=20)
    fig.set_size_inches(30, 15)
    ax.xaxis.label.set_size(24)
    ax.yaxis.label.set_size(24)
    
    plt.legend()
    plt.savefig("probability_entropy_similarity_grouped.pdf") 


def plot_cumulative_entropy_similarity_by_threshold(block_entropies_similarity, entropy):
    max_entropy_diff = 1
    step=0.05
    buckets = np.arange(0, max_entropy_diff + step, step)

    fig, ax = plt.subplots(figsize=(30, 30))

    count, bins_count = np.histogram(block_entropies_similarity[entropy], bins=buckets)
    pdf = count / sum(count)
    cdf = np.cumsum(pdf)
    ax.plot(bins_count[1:], cdf, alpha=0.5, label="Threshold = {}".format(entropy))

    plt.xlabel('Entropy similarity')
    plt.ylabel('Cumulative probability')

    ax.tick_params(axis='both', which='major', labelsize=20)
    fig.set_size_inches(30, 15)
    ax.xaxis.label.set_size(24)
    ax.yaxis.label.set_size(24)
    
    plt.legend()
    plt.savefig("cumulative_entropy_similarity{}.pdf".format(entropy)) 


def plot_cumulative_entropy_similarity(block_entropies_similarity):
    max_entropy_diff = 1
    step=0.05
    buckets = np.arange(0, max_entropy_diff + step, step)

    fig, ax = plt.subplots(figsize=(30, 30))

    for i in range(0, 8):
        count, bins_count = np.histogram(block_entropies_similarity[i], bins=buckets)
        pdf = count / sum(count)
        cdf = np.cumsum(pdf)
        ax.plot(bins_count[1:], cdf, alpha=0.5, label="Threshold = {}".format(i))

    plt.xlabel('Entropy similarity')
    plt.ylabel('Cumulative probability')

    ax.tick_params(axis='both', which='major', labelsize=20)
    fig.set_size_inches(30, 15)
    ax.xaxis.label.set_size(24)
    ax.yaxis.label.set_size(24)
    
    plt.legend()
    plt.savefig("cumulative_entropy_similarity.pdf") 


def plot_cumulative_entropy_similarity_grouped(block_entropies_similarity):
    max_entropy_diff = 1
    step=0.05
    buckets = np.arange(0, max_entropy_diff + step, step)

    fig, ax = plt.subplots(figsize=(30, 30))

    for i in [0, 4, 7]:
        count, bins_count = np.histogram(block_entropies_similarity[i], bins=buckets)
        pdf = count / sum(count)
        cdf = np.cumsum(pdf)
        ax.plot(bins_count[1:], cdf, alpha=0.5, label="Threshold = {}".format(i))

    plt.xlabel('Entropy similarity')
    plt.ylabel('Cumulative probability')

    ax.tick_params(axis='both', which='major', labelsize=20)
    fig.set_size_inches(30, 15)
    ax.xaxis.label.set_size(24)
    ax.yaxis.label.set_size(24)
    
    plt.legend()
    plt.savefig("cumulative_entropy_similarity_grouped.pdf") 


def plot_cumulative_entropy_differential_by_threshold(block_entropies_similarity, threshold):
    max_entropy_diff = 1
    step=0.05
    buckets = np.arange(0, max_entropy_diff + step, step)

    fig, ax = plt.subplots(figsize=(30, 30))

    count, bins_count = np.histogram(block_entropies_similarity[threshold], bins=buckets)
    pdf = count / sum(count)
    cdf = np.cumsum(pdf)
    ax.plot(bins_count[1:], cdf, label="Threshold = {}".format(threshold))

    plt.xlabel('Entropy differential')
    plt.ylabel('Cumulative probability')

    ax.tick_params(axis='both', which='major', labelsize=20)
    fig.set_size_inches(30, 15)
    ax.xaxis.label.set_size(24)
    ax.yaxis.label.set_size(24)
    
    plt.legend()
    plt.savefig("cumulative_entropy_differential{}.pdf".format(threshold))


def plot_cumulative_entropy_differential(block_entropies_similarity):
    max_entropy_diff = 1
    step=0.05
    buckets = np.arange(0, max_entropy_diff + step, step)

    fig, ax = plt.subplots(figsize=(30, 30))

    # for i in range(0, 8):
    for i in range(0, 8):
        count, bins_count = np.histogram(block_entropies_similarity[i], bins=buckets)
        pdf = count / sum(count)
        cdf = np.cumsum(pdf)
        ax.plot(bins_count[1:], cdf, label="Threshold = {}".format(i))

    plt.xlabel('Entropy differential')
    plt.ylabel('Cumulative probability')

    ax.tick_params(axis='both', which='major', labelsize=20)
    fig.set_size_inches(30, 15)
    ax.xaxis.label.set_size(24)
    ax.yaxis.label.set_size(24)
    
    plt.legend()
    plt.savefig("cumulative_entropy_differential.pdf") 


def plot_cumulative_entropy_differential_grouped(block_entropies_similarity):
    max_entropy_diff = 1
    step=0.05
    buckets = np.arange(0, max_entropy_diff + step, step)

    fig, ax = plt.subplots(figsize=(30, 30))

    # for i in range(0, 8):
    for i in [0, 4, 7]:
        count, bins_count = np.histogram(block_entropies_similarity[i], bins=buckets)
        pdf = count / sum(count)
        print("pdf: " + str(pdf))
        cdf = np.cumsum(pdf)
        ax.plot(bins_count[1:], cdf, label="Threshold = {}".format(i))

    plt.xlabel('Entropy differential')
    plt.ylabel('Cumulative probability')

    ax.tick_params(axis='both', which='major', labelsize=20)
    fig.set_size_inches(30, 15)
    ax.xaxis.label.set_size(24)
    ax.yaxis.label.set_size(24)
    
    plt.legend()
    plt.savefig("cumulative_entropy_differential_grouped.pdf") 


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


def count_modified_blocks(before_results_file_path, after_results_file_path, threshold):
    counter = 0
    
    with open(CUR_DIR + before_results_file_path, "r") as before_results_file, open(CUR_DIR + after_results_file_path, "r") as after_results_file: 
        before_results_content = before_results_file.readlines()
        after_results_content = after_results_file.readlines()

        for i in range(0, len(before_results_content)):
            if "PLAINTEXT" not in before_results_content[i] and "IMAGE" not in before_results_content[i] and "COMPRESSED" not in before_results_content[i] and "ENCRYPTED" not in before_results_content[i]:
                before_parsed_entropy = before_results_content[i].split('\n')[0]
                before_entropy = float(before_parsed_entropy)
                
                after_parsed_entropy = after_results_content[i].split('\n')[0]
                after_entropy = float(after_parsed_entropy)

                if before_entropy >= threshold:
                    counter += 1
                else:
                    if threshold == 0:
                        print("before entropy: " + str(before_entropy) + " after entropy:" + str(after_entropy))
                        print("BLOCK DID NOT CHANGE: " + str(i + 1) + " -> " + str(abs(before_entropy - after_entropy)))

    # for diff in block_entropies_diffs[threshold]:
    #     if diff > 0:
    #         counter += 1
    return counter


def table_entropy_statistics(before_results_file_path, after_results_file_path, block_entropies_diffs):
    with open("entropy_statistics_table.txt", "w") as results_file:
        column_frmt = ['p{1.2cm}', 'R{2.5cm}', 'R{2.2cm}', 'R{2.2cm}', 'R{2.2cm}', 'R{2.2cm}', 'R{2.2cm}']
        title_cols = ['Threshold', "Modified block count", "Usable blocks (\%)", "Storage Capacity (GB)", "Average differential", "Minimum differential", "Maximum differential"]
        write_latex_table_header(results_file, LEFT_MARGIN, RIGHT_MARGIN, column_frmt, title_cols)
        results_file.write("\t\t\\toprule\n")

        print("blocks_in_a_gb: " + str(blocks_in_a_gb))
        for i in range(0, 8):
            count_usable_blocks = count_modified_blocks(before_results_file_path, after_results_file_path[i], i)
            line_contents = []
            line_contents.append(str(i))
            line_contents.append(count_usable_blocks)
            line_contents.append("{:.2f}\%".format(count_usable_blocks / total_blocks_in_virtual_disks * 100)) 
            line_contents.append("{:.2f}".format((count_usable_blocks * 4096) / (1024 * 1024 * 1024))) 
            line_contents.append("{:.5f}".format(np.average(block_entropies_diffs[i])))
            line_contents.append("{:.5f}".format(min(block_entropies_diffs[i])))
            line_contents.append("{:.5f}".format(max(block_entropies_diffs[i])))
            write_latex_table_content_line(results_file, line_contents)

            if i < 7:
                results_file.write("\t\t\\midrule\n")

        write_latex_table_footer(results_file, 'Differential entropy statistics', 'tab:entropy_statistics_table')


# All values in a list are translated into 0 - 1 values
def normalize_lst_values(lst_before, norm):
    # norm = np.linalg.norm(lst_before)
    # print("norm: " + str(norm))
    # if norm <= 0:
    #     print(lst_before[0 : 100])
    lst_before_np = np.array(lst_before)
    # norm = 8
    lst_after = lst_before_np / norm
    return lst_after


def plot_differential_entropy(before_results_file_path, after_results_file_path, plot_file_name):
    block_entropies_before = [] 
    block_entropies_after = []
    block_entropies_diffs = [] # y values
    block_entropies_similarities = [] # y values
    x_values = []

    with open(CUR_DIR + before_results_file_path, "r") as results_file: 
        results_content = results_file.readlines()

        for line in results_content:
            if "PLAINTEXT" not in line and "IMAGE" not in line and "COMPRESSED" not in line and "ENCRYPTED" not in line:
                parsed_entropy = line.split('\n')[0]
                entropy = float(parsed_entropy)
                block_entropies_before.append(entropy)

    for i in range(0, len(block_entropies_before)):
        x_values.append(i)
    
    for i in range(0, 8):
        block_entropies_after.append([])
        block_entropies_diffs.append([])
        block_entropies_similarities.append([])

        with open(CUR_DIR + after_results_file_path[i], "r") as results_file: 
            results_content = results_file.readlines()

            for line in results_content:
                if "PLAINTEXT" not in line and "IMAGE" not in line and "COMPRESSED" not in line and "ENCRYPTED" not in line:
                    parsed_entropy = line.split('\n')[0]
                    entropy = float(parsed_entropy)
                    block_entropies_after[i].append(entropy)   

        # print("block_entropies_after: " + str(block_entropies_after))
        for j in range(0, len(block_entropies_before)):
            block_entropies_diffs[i].append(math.fabs(block_entropies_after[i][j] - block_entropies_before[j]))

    flat_entropy_diffs = np.array(block_entropies_diffs).flatten()
    norm_val = max(flat_entropy_diffs)
    for i in range(0, 8):
        # TODO: see if normalization should be done to all or individually by threshold
        block_entropies_diffs[i] = normalize_lst_values(block_entropies_diffs[i], norm_val)

        for j in range(0, len(block_entropies_before)):
            block_entropies_similarities[i].append(1 - math.fabs(block_entropies_diffs[i][j]))


    # fig, ax = plt.subplots(figsize=(30, 4.5))
    
    # for i in range(0, 8):
    #     ax.plot(x_values, block_entropies_diffs[i], linewidth=0.5)

    # plt.xlabel('Block index')
    # plt.ylabel('Entropy differential')
    # x_ticks = np.arange(0, total_blocks_in_virtual_disks + 100000, 100000)
    # plt.xticks(x_ticks)
    # plt.yticks(np.arange(0, 1.1, 1))

    # str_x_ticks = []
    # for block_nr in x_ticks:
    #     str_x_ticks.append("{:.2e}".format(block_nr))
    # ax.set_xticklabels(str_x_ticks)

    # plt.legend(loc='upper right', labels=['threshold = 0', 'threshold = 1', 'threshold = 2', 'threshold = 3', 'threshold = 4', 'threshold = 5', 'threshold = 6', 'threshold = 7'])
    # plt.savefig(plot_file_name)  


    # fig2, ax2 = plt.subplots(figsize=(30, 4))
    # ax2.plot(x_values, block_entropies_diffs[0], linewidth=0.5)
    # plt.xlabel('Block index')
    # plt.ylabel('Entropy differential')
    # plt.xticks(x_ticks)
    # plt.yticks(np.arange(0, 1.1, 1))
    # ax2.set_xticklabels(str_x_ticks)
    # plt.savefig("entropy_differential_plot_threshold_0.pdf") 

    # fig3, ax3 = plt.subplots(figsize=(30, 4))
    # ax3.plot(x_values, block_entropies_diffs[7], linewidth=0.5)
    # plt.xlabel('Block index')
    # plt.ylabel('Entropy differential')
    # plt.xticks(x_ticks)
    # plt.yticks(np.arange(0, 1.1, 1))
    # ax3.set_xticklabels(str_x_ticks)
    # plt.savefig("entropy_differential_plot_threshold_7.pdf") 

    # fig4, ax4 = plt.subplots(figsize=(30, 4))
    # ax4.plot(x_values, block_entropies_diffs[6], linewidth=0.5)
    # plt.xlabel('Block index')
    # plt.ylabel('Entropy differential')
    # plt.xticks(x_ticks)
    # plt.yticks(np.arange(0, 1.1, 1))
    # ax4.set_xticklabels(str_x_ticks)
    # plt.savefig("entropy_differential_plot_threshold_6.pdf") 

    # fig5, ax5 = plt.subplots(figsize=(30, 4))
    # ax5.plot(x_values, block_entropies_diffs[5], linewidth=0.5)
    # plt.xlabel('Block index')
    # plt.ylabel('Entropy differential')
    # plt.xticks(x_ticks)
    # plt.yticks(np.arange(0, 1.1, 1))
    # ax5.set_xticklabels(str_x_ticks)
    # plt.savefig("entropy_differential_plot_threshold_5.pdf") 

    # fig6, ax6 = plt.subplots(figsize=(30, 4))
    # ax6.plot(x_values, block_entropies_diffs[4], linewidth=0.5)
    # plt.xlabel('Block index')
    # plt.ylabel('Entropy differential')
    # plt.xticks(x_ticks)
    # plt.yticks(np.arange(0, 1.1, 1))
    # ax6.set_xticklabels(str_x_ticks)
    # plt.savefig("entropy_differential_plot_threshold_4.pdf") 

    
    # Uncomment
    # table_entropy_statistics(before_results_file_path, after_results_file_path, block_entropies_diffs)
    
    # plot_cumulative_entropy(block_entropies_before)

    # plot_probability_entropy_similarity(block_entropies_similarities)
    # plot_probability_entropy_similarity_grouped(block_entropies_similarities)
    # for i in range(0, 8):
    #     plot_probability_entropy_similarity_by_threshold(block_entropies_similarities, i)
    
    # plot_cumulative_entropy_similarity(block_entropies_similarities)
    # plot_cumulative_entropy_similarity_grouped(block_entropies_similarities)
    # for i in range(0, 8):
    #     plot_cumulative_entropy_similarity_by_threshold(block_entropies_similarities, i)

    # plot_cumulative_entropy_differential(block_entropies_diffs)
    plot_cumulative_entropy_differential_grouped(block_entropies_diffs)
    # for i in range(0, 8):
    #     plot_cumulative_entropy_differential_by_threshold(block_entropies_diffs, i)

    # waffle_chart_diffs(block_entropies_diffs)
    waffle_chart_diffs2(block_entropies_diffs, before_results_file_path, after_results_file_path)


# $ python3 plot_entropy_before_after_usable_blocks.py
if __name__ == '__main__':
    plot_differential_entropy(RESULTS_FILE_BEFORE, RESULTS_FILES_AFTER, PLOT_FILE_NAME)