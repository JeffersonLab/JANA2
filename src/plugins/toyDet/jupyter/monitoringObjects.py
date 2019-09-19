#!/usr/bin/python3.6

import random
import numpy as np
import matplotlib.pyplot as plt
from IPython.display import display, clear_output, set_matplotlib_close

from ipywidgets import HBox

# define global variables
numChans = 80


class OccupancyFig:
    """Class to create figure for occupancy plots"""

    # define figure and plot attributes
    def __init__(self):
        self.fig = plt.figure()
        self.fig.set_size_inches(18.5, 10.5, forward=True)


class OccupancyPlot:
    """Class to plot streaming occupancy data"""

    def __init__(self, dataDict, thresh, fig, hitCounter):
        self.hitThresh  = thresh
        self.fig = fig
        self.hitCounter = hitCounter
        self.chanList = np.arange(1, numChans + 1, 1)
        for chan in range(1, numChans + 1):
            if len(np.where(dataDict['adcSamplesChan_%d' % chan] > self.hitThresh)[0]) != 0:
                self.hitCounter[chan-1] += 1
            # remove event data after being published
            dataDict.pop('adcSamplesChan_%s' % str(chan), None)
            dataDict.pop('tdcSamplesChan_%s' % str(chan), None)
        plt.bar(self.chanList, self.hitCounter, align='center')
        plt.xlabel('Channel Number')
        plt.ylabel('Number of ADC Hits > %d Channels' % self.hitThresh)
        plt.title('ADC Occupancy')
        set_matplotlib_close(False)
        display(self.fig)
        clear_output(wait = False)
        plt.pause(0.05)

    def get_hit_cntr(self):
        return self.hitCounter


class WaveFormFig:
    """Class to create a figure and subplots based on user input for viewing waveforms"""

    # define figure and plot attributes
    def __init__(self, rows, cols):
        self.numRows = rows
        self.numCols = cols
        self.fig, self.axs = plt.subplots(self.numRows, self.numCols)
        self.fig.set_size_inches(18.5, 10.5, forward=True)
        # channel list, ascending and non-repeating
        self.chans = random.sample(range(1, numChans + 1), self.numRows * self.numCols)
        self.chans.sort()

    def get_num_rows(self):
        return self.numRows

    def get_num_cols(self):
        return self.numCols

    def get_fig_obj(self):
        return self.fig

    def get_axs_obj(self):
        return self.axs

    def get_chan_list(self):
        return self.chans


class WaveFormPlot:
    """Class to plot streaming ADC vs. TDC data"""

    # instance variables unique to each instance
    def __init__(self, dataDict, thresh, rows, cols, fig, axs, chans):
        # figure parameters
        self.hitThresh = thresh
        self.numRows = rows
        self.numCols = cols
        self.fig = fig
        self.axs = axs
        self.chans = chans
        # lists for colors and markers
        self.cl = ['tab:blue', 'tab:orange', 'tab:green', 'tab:red', 'tab:purple',
                   'tab:brown', 'tab:pink', 'tab:gray', 'tab:olive', 'tab:cyan']
        self.ml = ['o', '^', 's', 'p', 'P', '*', 'X', 'd']
        # event number list, index counter
        self.enl = []
        self.ic = 0
        for row in range(self.numRows):
            for column in range(self.numCols):
                self.axs[row, column].cla()
                self.axs[row, column].plot(dataDict['tdcSamplesChan_%d' % self.chans[self.ic]],
                                           dataDict['adcSamplesChan_%d' % self.chans[self.ic]],
                                           color=self.cl[self.ic % len(self.cl)],
                                           marker=self.ml[self.ic % len(self.ml)],
                                           ls='', label='Channel %d' % self.chans[self.ic])
                hitLoc = np.where(dataDict['adcSamplesChan_%d' % self.chans[self.ic]] > self.hitThresh)[0] + \
                         np.min(dataDict['tdcSamplesChan_%d' % self.chans[self.ic]])
                if len(hitLoc) != 0: self.axs[row, column].set_xlim(np.min(hitLoc) - 10, np.max(hitLoc) + 20)
                self.axs[row, column].set_ylim(0, 1024)
                if column == 0:
                    self.axs[row, column].set_ylabel('ADC Value')
                if row == self.numRows - 1:
                    self.axs[row, column].set_xlabel('TDC Sample Number')
                self.axs[row, column].legend(loc='best', markerscale=0, handletextpad=0, handlelength=0)
                self.ic += 1
        # plt.tight_layout()
        # plt.savefig('plots/event_%d.png' % ec)
        # plt.pause(0.05)
        plt.tight_layout()
        set_matplotlib_close(False)
        display(self.fig)
        clear_output(wait = False)
        plt.pause(0.05)
        # remove event data after being published
        for chan in range(1, numChans + 1):
            dataDict.pop('adcSamplesChan_%s' % str(chan), None)
            dataDict.pop('tdcSamplesChan_%s' % str(chan), None)
