#!/usr/bin/python3.6

import zmq
from indraMessage import IndraMessage
from monitoringObjects import MonitoringFigure, MonitoringPlots


class StreamSubscriber:
    """Class to handle INDRA Streaming"""

    def __init__(self, port, nrows, ncols, thresh):
        self.subPort = port
        self.numRows = nrows
        self.numCols = ncols
        self.hitThresh = thresh
        self.subContext = zmq.Context()
        self.subscriber = self.subContext.socket(zmq.SUB)
        self.subscriber.setsockopt(zmq.SUBSCRIBE, b'')
        self.subscriber.connect('tcp://127.0.0.1:%d' % self.subPort)
        print('\nSubscribing to JANA ZMQ Messages on socket tcp://127.0.0.1:%d\n' % self.subPort)
        # instantiate the monitoring figure class
        self.monFig = MonitoringFigure(self.numRows, self.numCols)
        # receive zmq messages from jana publisher
        while True:
            # receive zmq packets from jana publisher
            self.janaMessage = self.subscriber.recv()
            # instantiate the indra message class
            self.jevent = IndraMessage(self.janaMessage)
            # instantiate the monitoring plot class
            self.monPlots = MonitoringPlots(self.jevent.dataDict, self.hitThresh,
                                            self.monFig.get_num_rows(), self.monFig.get_num_cols(),
                                            self.monFig.get_fig_obj(), self.monFig.get_axs_obj())
            # self.figCanvas = self.monFig.get_fig_obj().canvas
