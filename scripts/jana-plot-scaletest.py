#!/usr/bin/env python3
#
# Copyright 2020, Jefferson Science Associates, LLC.
# Subject to the terms in the LICENSE file found in the top-level directory.


import os
import sys

fname = 'rates.dat'
NO_GRAPHICS_WINDOW = False
MAKE_PNG = False

if '-nowin' in sys.argv[1:] : NO_GRAPHICS_WINDOW = True
if '-png'   in sys.argv[1:] : MAKE_PNG = True

# Make sure the rates.dat file exists and print usage statement otherwise
if not os.path.exists(fname):
    print()
    print('Cannot find file: ' + fname + '!')
    print('''

Usage:
    jana-plot-scaletest.py [-png] [-nowin]

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

   BENCHMARK:NSAMPLES     # Number of samples for each benchmark test
   BENCHMARK:MINTHREADS   # Minimum number of threads for benchmark test
   BENCHMARK:MAXTHREADS   # Maximum number of threads for benchmark test
   BENCHMARK:RESULTSDIR   # Output directory name for benchmark test results
   BENCHMARK:THREADSTEP   # Delta number of threads between each benchmark test

 Run "jana -c -b" to see the default values for each of these.

 To use this just go into the BENCHMARK:RESULTSDIR where the
 rates.dat file is and run it. Note that you must have python
 installed as well as the numpy and matplotlib python packages.

 If you wish to just create the plot as a PNG file without opening
 a window (convenient for machines where a graphical environment
 isn't present), then give it the -nowin argument like this:

    jana-plot-scaletest.py -nowin
    ''')
    sys.exit(0)

import numpy as np
import matplotlib
from datetime import datetime


if NO_GRAPHICS_WINDOW:
    matplotlib.use('Agg')
    MAKE_PNG = True  # Always make PNG file if user specifies not to open a graphics window

# NOTE: matplotlib.pyplot must be imported AFTER matplotlib.use(...) is called
import matplotlib.pyplot as plt


# Load data using numpy
nthreads,avg_rate,rms_rate = np.loadtxt(fname, skiprows=1,usecols=(0,1,2), unpack=True)
minnthreads = nthreads.min()
maxnthreads = nthreads.max()

# Create plot using matplotlib
plt.errorbar(nthreads, avg_rate, rms_rate, linestyle='', ecolor='red', elinewidth=3, capthick=3, marker='o', ms=8, markerfacecolor='green')

creation_time = int(os.path.getctime(fname))
plt.title('JANA JTest Scaling Test : ' + str(datetime.fromtimestamp(creation_time)))
plt.xlabel('Nthreads')
plt.ylabel('Rate (Hz)')

plt.xlim(minnthreads-1.0, maxnthreads+1.0)
plt.grid(True)


if MAKE_PNG:
    png_filename = 'rate_vs_nthreads.png'
    print('Saving plot to: ' + png_filename)
    plt.savefig(png_filename)

if not NO_GRAPHICS_WINDOW:
    plt.show()

