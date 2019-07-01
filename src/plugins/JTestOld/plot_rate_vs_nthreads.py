#!/usr/bin/env python
#
# This script can be used to plot the results of running the
# JANA JTest plugin in scaling test mode. To use it, you must
# have python installed as well as the numpy and matplotlib
# python packages. Just run it in the same directory as the
# rates.dat file.
# 
# n.b. This will automatically add the creation time of the
# rates.dat file to the plot title. Be cautious copying it!
#

import os
import numpy as np
import matplotlib.pyplot as plt
from datetime import datetime

fname = 'rates.dat'

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

plt.show()
