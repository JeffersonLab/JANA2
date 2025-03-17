#!/usr/bin/env python3
#
# Copyright 2020, Jefferson Science Associates, LLC.
# Subject to the terms in the LICENSE file found in the top-level directory.

import os
import sys
import toml
import numpy as np
import matplotlib
import seaborn

def make_plot(plot_spec):

    plt.title(plot_spec['title'])
    plt.xlabel('Nthreads')
    plt.ylabel('Rate (Hz)')
    #plt.rcParams['text.usetex'] = True
    plt.grid(True)

    legend = []
    for subplot in plot_spec["test"]:

        # Load data using numpy
        nthreads,avg_rate,rms_rate = np.loadtxt(subplot['datafile'], skiprows=1, usecols=(0,1,2), unpack=True)
        minnthreads = nthreads.min()
        maxnthreads = nthreads.max()
        #plt.xlim(minnthreads-1.0, 10) #maxnthreads+1.0)

        tpar = subplot['t_par']
        tseq = subplot['t_seq']
        nthreads = np.arange(minnthreads, maxnthreads+1, 1)
        seq_bottleneck = 1000 / tseq
        par_bottleneck = 1000 * nthreads / tpar
        amdahl_ys = 1000 / (tseq + (tpar/nthreads))
        tputs = np.minimum(par_bottleneck, seq_bottleneck)
        print(tputs)

        # Create plot using matplotlib
        plt.errorbar(nthreads, avg_rate, rms_rate, linestyle='', ecolor='gray', elinewidth=2, capthick=3, marker='o', ms=4, markeredgecolor=subplot['color'], markerfacecolor=subplot['color'], label=subplot['key'])
        plt.plot(nthreads, tputs, linestyle=":", color=subplot['color'])

    plt.legend()
    return plt


if __name__ == "__main__":

    fname = 'plot.toml'
    no_window = False
    make_png = False

    if '-nowin' in sys.argv[1:] : no_window = True
    if '-png'   in sys.argv[1:] : make_png = True

    # Make sure the rates.dat file exists and print usage statement otherwise
    if not os.path.exists(fname):
        print()
        print('Cannot find file: ' + fname + '!')
        print('''

    Usage:
        jana-plot-scaletests.py [-png] [-nowin] TOMLFILE

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

    if no_window:
        matplotlib.use('Agg')
        make_png = True  # Always make PNG file if user specifies not to open a graphics window

    # matplotlib.pyplot must be imported AFTER matplotlib.use(...) is called
    import matplotlib.pyplot as plt

    spec = toml.load(fname)
    plt = make_plot(spec)

    if make_png:
        png_filename = 'rate_vs_nthreads.png'
        print('Saving plot to: ' + png_filename)
        plt.savefig(png_filename)

    if not no_window:
        plt.show()




