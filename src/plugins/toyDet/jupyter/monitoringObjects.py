#!/usr/bin/python3.6

import random
import numpy as np
import matplotlib.pyplot as plt
from IPython.display import display, clear_output, set_matplotlib_close

# define global variables
numChans = 80


class OccupancyFig:
    """Class to create figure for occupancy plots"""

    # define figure and plot attributes
    def __init__(self):
        self.fig = plt.figure()
        # self.fig.set_size_inches(18.5, 10.5, forward=True)


class OccupancyPlot:
    """Class to plot streaming occupancy data"""

    def __init__(self, data_dict, thresh, fig, hit_cntr):
        self.thresh = thresh
        self.fig = fig
        self.hit_cntr = hit_cntr
        self.chan_list = np.arange(1, numChans + 1, 1)
        for chan in range(1, numChans + 1):
            if len(np.where(data_dict['adcSamplesChan_%d' % chan] > self.thresh)[0]) != 0:
                self.hit_cntr[chan-1] += 1
            # remove event data after being published
            data_dict.pop('adcSamplesChan_%s' % str(chan), None)
            data_dict.pop('tdcSamplesChan_%s' % str(chan), None)
        plt.bar(self.chan_list, self.hit_cntr, align = 'center', color = 'tab:blue')
        plt.xlabel('Channel Number')
        plt.ylabel('Number of ADC Hits > %d Channels' % self.thresh)
        plt.title('ADC Occupancy')
        # display(self.fig)
        clear_output(wait = True)
        plt.pause(0.005)

    def get_hit_cntr(self):
        return self.hit_cntr


class WaveformFig:
    """Class to create a figure and subplots based on user input for viewing waveforms"""

    # define figure and plot attributes
    def __init__(self, nrows, ncols):
        self.nrows = nrows
        self.ncols = ncols
        self.fig, self.axs = plt.subplots(self.nrows, self.ncols)
        # self.fig.set_size_inches(18.5, 10.5, forward=True)
        # channel list, ascending and non-repeating
        self.chans = random.sample(range(1, numChans + 1), self.nrows * self.ncols)
        self.chans.sort()

    def get_num_rows(self):
        return self.nrows

    def get_num_cols(self):
        return self.ncols

    def get_fig_obj(self):
        return self.fig

    def get_axs_obj(self):
        return self.axs

    def get_chan_list(self):
        return self.chans


class WaveformPlot:
    """Class to plot streaming ADC vs. TDC data"""

    # instance variables unique to each instance
    def __init__(self, data_dict, thresh, nrows, ncols, fig, axs, chans):
        # figure parameters
        self.thresh = thresh
        self.nrows = nrows
        self.ncols = ncols
        self.fig = fig
        self.axs = axs
        self.chans = chans
        # lists for colors and markers
        self.cl = ['tab:blue', 'tab:orange', 'tab:green', 'tab:red', 'tab:purple',
                   'tab:brown', 'tab:pink', 'tab:gray', 'tab:olive', 'tab:cyan']
        self.ml = ['o', '^', 's', 'p', 'P', '*', 'X', 'd']
        self.ic = 0  # index counter
        if self.nrows == 1 and self.ncols == 1:
            self.axs.cla()
            self.axs.plot(data_dict['tdcSamplesChan_%d' % self.chans[self.ic]],
                          data_dict['adcSamplesChan_%d' % self.chans[self.ic]],
                          color = self.cl[self.ic % len(self.cl)],
                          marker = self.ml[self.ic % len(self.ml)],
                          ls = '', label = 'Channel %d' % self.chans[self.ic])
            hit_loc = np.where(data_dict['adcSamplesChan_%d' % self.chans[self.ic]] > self.thresh)[0] + \
                      np.min(data_dict['tdcSamplesChan_%d' % self.chans[self.ic]])
            if len(hit_loc) != 0: self.axs.set_xlim(np.min(hit_loc) - 10, np.max(hit_loc) + 20)
            self.axs.set_ylim(0, 1024)
            self.axs.set_ylabel('ADC Value')
            self.axs.set_xlabel('TDC Sample Number')
            self.axs.legend(loc = 'best', markerscale = 0, handletextpad = 0, handlelength = 0)
            self.ic += 1
        if (self.nrows == 1 and self.ncols == 2) or (self.nrows == 2 and self.ncols == 1):
            for index in range(2):
                self.axs[index].cla()
                self.axs[index].plot(data_dict['tdcSamplesChan_%d' % self.chans[self.ic]],
                              data_dict['adcSamplesChan_%d' % self.chans[self.ic]],
                              color = self.cl[self.ic % len(self.cl)],
                              marker = self.ml[self.ic % len(self.ml)],
                              ls = '', label = 'Channel %d' % self.chans[self.ic])
                hit_loc = np.where(data_dict['adcSamplesChan_%d' % self.chans[self.ic]] > self.thresh)[0] + \
                          np.min(data_dict['tdcSamplesChan_%d' % self.chans[self.ic]])
                if len(hit_loc) != 0: self.axs[index].set_xlim(np.min(hit_loc) - 10, np.max(hit_loc) + 20)
                self.axs[index].set_ylim(0, 1024)
                self.axs[index].set_ylabel('ADC Value')
                self.axs[index].set_xlabel('TDC Sample Number')
                self.axs[index].legend(loc = 'best', markerscale = 0, handletextpad = 0, handlelength = 0)
                self.ic += 1
        if self.nrows >= 2 and self.ncols >=2:
            for row in range(self.nrows):
                for column in range(self.ncols):
                    self.axs[row, column].cla()
                    self.axs[row, column].plot(data_dict['tdcSamplesChan_%d' % self.chans[self.ic]],
                                               data_dict['adcSamplesChan_%d' % self.chans[self.ic]],
                                               color = self.cl[self.ic % len(self.cl)],
                                               marker = self.ml[self.ic % len(self.ml)],
                                               ls = '', label = 'Channel %d' % self.chans[self.ic])
                    hit_loc = np.where(data_dict['adcSamplesChan_%d' % self.chans[self.ic]] > self.thresh)[0] + \
                             np.min(data_dict['tdcSamplesChan_%d' % self.chans[self.ic]])
                    if len(hit_loc) != 0: self.axs[row, column].set_xlim(np.min(hit_loc) - 10, np.max(hit_loc) + 20)
                    self.axs[row, column].set_ylim(0, 1024)
                    if column == 0:
                        self.axs[row, column].set_ylabel('ADC Value')
                    if row == self.nrows - 1:
                        self.axs[row, column].set_xlabel('TDC Sample Number')
                    self.axs[row, column].legend(loc = 'best', markerscale = 0, handletextpad = 0, handlelength = 0)
                    self.ic += 1
        plt.tight_layout()
        # plt.savefig('plots/event_%d.png' % ec)
        # plt.pause(0.05)
        # display(self.fig)
        set_matplotlib_close(False)
        clear_output(wait = True)
        plt.pause(0.005)
        # remove event data after being published
        for chan in range(1, numChans + 1):
            data_dict.pop('adcSamplesChan_%s' % str(chan), None)
            data_dict.pop('tdcSamplesChan_%s' % str(chan), None)
