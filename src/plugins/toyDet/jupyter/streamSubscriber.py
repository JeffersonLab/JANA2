#!/usr/bin/python3.6

import zmq, time
import numpy as np
from indraMessage import IndraMessage
from monitoringObjects import *

# define global variables
numChans = 80


class StreamSubscriber:
    """Class to handle INDRA Streaming"""

    # def __init__(self, port, nrows, ncols, thresh):
    # def __init__(self, port, thresh):
    def __init__(self, type = 'occ', port = 5557, thresh = 100, nrows = 2, ncols = 2):
        self.plotType = type
        self.subPort = port
        self.hitThresh = thresh
        self.numRows = nrows
        self.numCols = ncols
        self.subContext = zmq.Context()
        self.subscriber = self.subContext.socket(zmq.SUB)
        self.subscriber.setsockopt(zmq.SUBSCRIBE, b'')
        self.subscriber.connect('tcp://127.0.0.1:%d' % self.subPort)
        print('\nSubscribing to JANA ZMQ Messages on socket tcp://127.0.0.1:%d\n' % self.subPort)
        # instantiate the monitoring figure class
        if self.plotType == 'occ':
            self.monFig = OccupancyFig()
            self.hitCounter = np.zeros(numChans)
        if self.plotType == 'wf':
            self.monFig = WaveFormFig(self.numRows, self.numCols)
        # receive zmq messages from jana publisher
        while True:
            # receive zmq packets from jana publisher
            self.janaMessage = self.subscriber.recv()
            # instantiate the indra message class
            self.jevent = IndraMessage(self.janaMessage)
            # instantiate the monitoring plot classes
            if self.plotType == 'occ':
                self.monPlots = OccupancyPlot(self.jevent.dataDict, self.hitThresh, self.monFig, self.hitCounter)
            if self.plotType == 'wf':
                self.monPlots = WaveFormPlot(self.jevent.dataDict, self.hitThresh, self.monFig.get_num_rows(),
                                             self.monFig.get_num_cols(), self.monFig.get_fig_obj(),
                                             self.monFig.get_axs_obj(), self.monFig.get_chan_list())
