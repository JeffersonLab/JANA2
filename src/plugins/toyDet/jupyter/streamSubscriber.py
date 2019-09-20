#!/usr/bin/python3.6

import zmq
import numpy as np
from indraMessage import IndraMessage
from monitoringObjects import *

# define global variables
numChans = 80


class StreamSubscriber:
    """Class to handle INDRA Streaming"""

    def __init__(self, plot_type = 'Occupancy', port = 5557, thresh = 100, nrows = 2, ncols = 2):
        self.plot_type = plot_type
        self.port = port
        self.thresh = thresh
        self.nrows = nrows
        self.ncols = ncols
        self.context = zmq.Context()
        self.subscriber = self.context.socket(zmq.SUB)
        self.subscriber.setsockopt(zmq.SUBSCRIBE, b'')
        self.subscriber.connect('tcp://127.0.0.1:%d' % self.port)
        print('\nSubscribing to JANA ZMQ Messages on socket tcp://127.0.0.1:%d\n' % self.port)
        # instantiate the monitoring figure class
        if self.plot_type == 'Occupancy':
            self.mon_fig = OccupancyFig()
            self.hit_cntr = np.zeros(numChans)
        if self.plot_type == 'Waveform':
            self.mon_fig = WaveformFig(self.nrows, self.ncols)
        # receive zmq messages from jana publisher
        while True:
            # receive zmq packets from jana publisher
            self.jana_msg = self.subscriber.recv()
            # instantiate the indra message class
            self.jevent = IndraMessage(self.jana_msg)
            # instantiate the monitoring plot classes
            if self.plot_type == 'Occupancy':
                self.mon_plots = OccupancyPlot(self.jevent.data_dict, self.thresh, self.mon_fig, self.hit_cntr)
            if self.plot_type == 'Waveform':
                self.mon_plots = WaveformPlot(self.jevent.data_dict, self.thresh, self.mon_fig.get_num_rows(),
                                              self.mon_fig.get_num_cols(), self.mon_fig.get_fig_obj(),
                                              self.mon_fig.get_axs_obj(), self.mon_fig.get_chan_list())
