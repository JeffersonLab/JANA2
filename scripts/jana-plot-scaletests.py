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

def make_plot(plot_spec):

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

        nthreads,avg_rate,rms_rate = np.loadtxt(subplot['datafile'], skiprows=1, usecols=(0,1,2), unpack=True)
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
        plt.plot(nthreads, tputs, linestyle="-", color=color)
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

    This script can be used to plot the results of running the
    built in JANA scaling test. This test will automatically
    record the event processing rate as it cycles through changing
    the number of processing threads JANA is using. This can be
    done if using the "jana" executable by just giving it the
    -b argument. If using a custom executable you will need to
    replace the call to JApplication::Run() with:

    JBenchmarker benchmarker(app);
    benchmarker.RunUntilFinished();

    You can control several parameters of the test using these
    JANA configuration parameters:

    benchmark:nsamples     # Number of samples for each benchmark test
    benchmark:minthreads   # Minimum number of threads for benchmark test
    benchmark:maxthreads   # Maximum number of threads for benchmark test
    benchmark:resultsdir   # Output directory name for benchmark test results
    benchmark:threadstep   # Delta number of threads between each benchmark test

    Run "jana -c -b" to see the default values for each of these.

    To use this just go into the benchmark:resultsdir where the
    rates.dat file is and run it. Note that you must have python
    installed as well as the numpy and matplotlib python packages.

    If you wish to just create the plot as a PNG file without opening
    a window (convenient for machines where a graphical environment
    isn't present), then give it the -nowin argument like this:

        jana-plot-scaletest.py -nowin
        ''')
        sys.exit(0)

    spec = toml.load(input_filename)
    plt = make_plot(spec)
    output_filename = 'rate_vs_nthreads.pdf'
    if 'output_filename' in spec:
        output_filename = spec['output_filename']
    plt.savefig(output_filename)
    print('Saved plot to: ' + output_filename)



