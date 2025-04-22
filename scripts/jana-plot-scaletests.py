#!/usr/bin/env python3
#
# Copyright 2020, Jefferson Science Associates, LLC.
# Subject to the terms in the LICENSE file found in the top-level directory.

import os
import sys
import toml
import numpy as np
import matplotlib
import seaborn as sns


# In order to run with latex typesetting, you need:
# sudo dnf install texlive texlive-type1cm dvipng

def make_plot(plot_spec, input_dir):

    matplotlib.use('Agg')
    # matplotlib.pyplot must be imported AFTER matplotlib.use(...) is called
    import matplotlib.pyplot as plt

    if 'use_latex' in plot_spec:
        if plot_spec['use_latex']:
            plt.rcParams['text.usetex'] = True

    plt.title(plot_spec['title'])
    plt.xlabel('Nthreads')
    plt.ylabel('Rate (Hz)')
    plt.grid(True)
    colors = sns.color_palette("deep")
    colors_dark = sns.color_palette("dark")

    legend = []
    color_idx = 0
    for subplot in plot_spec["test"]:

        color = colors[color_idx]
        dark_color = colors_dark[color_idx]
        color_idx += 1

        subdatafile = subplot['datafile']
        if not os.path.isabs(subdatafile):
            subdatafile = os.path.join(input_dir, subdatafile)

        nthreads,avg_rate,rms_rate = np.loadtxt(subdatafile, skiprows=1, usecols=(0,1,2), unpack=True)
        minnthreads = nthreads.min()
        maxnthreads = nthreads.max()

        tpar = subplot['t_par']
        tseq = subplot['t_seq']
        nthreads = np.arange(minnthreads, maxnthreads+1, 1)
        seq_bottleneck = 1000 / tseq
        par_bottleneck = 1000 * nthreads / tpar
        amdahl_ys = 1000 / (tseq + (tpar/nthreads))
        tputs = np.minimum(par_bottleneck, seq_bottleneck)

        # Create plot using matplotlib
        plt.plot(nthreads, tputs, linestyle="-", linewidth=1, color=color)
        plt.errorbar(nthreads, avg_rate, rms_rate, 
                     linestyle='',
                     marker='o',
                     ecolor=dark_color, elinewidth=1, capsize=0, capthick=0,
                     markersize=2, markeredgewidth=1,
                     markeredgecolor=dark_color, markerfacecolor=color, label=subplot['key'])

    plt.legend()
    return plt


if __name__ == "__main__":

    input_filename = 'plot.toml'
    if len(sys.argv) > 1:
        input_filename = sys.argv[1]

    # Make sure the rates.dat file exists and print usage statement otherwise
    if not os.path.exists(input_filename):
        print()
        print('Cannot find file: ' + input_filename)
        print('''
    Usage:
        jana-plot-scaletests.py TOMLFILE

    This script plots multiple JANA scaling tests on the same axes. It assumes you
    use JANA2's built-in performance benchmarking tool to generate the scaling
    curves. The input argument is a TOML file with the following contents:

        - title: string
        - output_filename: string
        - use_latex: bool
        - test: list of dicts containing:
            - key: string              # Text used for the legend
            - datafile: string         # Relative path to a `rates.dat` file
            - t_seq: int               # Largest avg latency among sequential arrows
            - t_par: int               # Largest avg latency among parallel arrows

    Note: The output file format can be controlled by changing the extension on the
          output file name. The most useful outputs are `pdf` and `png`.

    Note: If you enable use_latex, you'll need to install texlive and a couple extra things.
          On my RPM-based distro, I needed `sudo dnf install texlive texlive-type1cm dvipng`

        ''')
        sys.exit(0)

    spec = toml.load(input_filename)
    input_dir = os.path.abspath(os.path.dirname(input_filename))
    plt = make_plot(spec, input_dir)
    output_filename = 'rate_vs_nthreads.pdf'
    if 'output_filename' in spec:
        output_filename = spec['output_filename']
    plt.savefig(output_filename)
    print('Saved plot to: ' + output_filename)



