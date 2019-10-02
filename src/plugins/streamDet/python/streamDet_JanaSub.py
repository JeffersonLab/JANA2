#!/usr/bin/python3.6

import zmq, struct, pickle, random
import numpy as np
import matplotlib.pyplot as plt

# define the subscriber and publisher ports
subPort = 5557
pubPort = 5558
# define global variables
numChans = 80

# configure the subscriber
subContext = zmq.Context()
subscriber = subContext.socket(zmq.SUB)
subscriber.setsockopt(zmq.SUBSCRIBE, b'')
subscriber.connect('tcp://127.0.0.1:%d' % subPort)
print('\nSubscribing to JANA ZMQ Messages on socket tcp://127.0.0.1:%d\n' % subPort)

# # configure the publisher
# pubContext = zmq.Context()
# publisher  = pubContext.socket(zmq.PUB)
# publisher.bind('tcp://127.0.0.1:%d' % pubPort)
# print('\nPublishing on tcp://127.0.0.1:%d\n'  % pubPort)


class IndraMessage:
    """Class to handle decoding of ZMQ INDRA Messages"""

    def __init__(self, zmqMessage):
        # def __init__(self, zmqMessage, zmqPublisher):
        # define the data dictionary
        self.dataDict = {}
        # size up the zmq message
        self.messageSize = len(zmqMessage)
        # indra message header is a fixed 56 bytes
        self.payloadBytes = self.messageSize - 56
        # unpack the message according to the indra message protocol
        self.message = struct.unpack('IIIIIIIQqq%ds' % self.payloadBytes, zmqMessage)
        # decode the indra message members
        self.sourceId = self.message[0]
        self.totalBytes = self.message[1]
        self.payloadBytes = self.message[2]
        self.compressedBytes = self.message[3]
        self.magic = self.message[4]
        self.formatVersion = self.message[5]
        self.flags = self.message[6]
        self.recordCounter = self.message[7]
        self.timeStampSec = self.message[8]
        self.timeStampNanoSec = self.message[9]
        self.payload = self.message[10]
        # print message info
        print('INDRA Message recieved -> event = %d, size = %d bytes' % (self.recordCounter, self.messageSize))
        # adc samples are uints of length 4 plus 1 space delimiter, convert to array of ints
        self.adcSamplesStr = np.frombuffer(self.payload, dtype='S5')
        self.adcSamples = np.reshape(self.adcSamplesStr.astype(np.int), (1024, 80))
        # define data dictionary keys and data types
        for chan in range(1, numChans + 1):
            self.dataDict.update({'adcSamplesChan_%s' % chan: np.array([])})
            self.dataDict.update(
                {'tdcSamplesChan_%s' % chan: np.arange((self.recordCounter - 1) * 1024, self.recordCounter * 1024)})
        # enumerate the samples object and populate the data dictionary
        for index, sample in np.ndenumerate(self.adcSamples):
            self.dataDict['adcSamplesChan_%s' % str(index[1] + 1)] = \
                np.append(self.dataDict['adcSamplesChan_%s' % str(index[1] + 1)], sample)
        # serialize the data dictionary via pickle and publish it
        # self.eventDataDict = pickle.dumps(self.dataDict)
        # zmqPublisher.send_pyobj(self.eventDataDict)
        # remove event data after being published
        # for chan in range(1, numChans + 1) :
        #     self.dataDict.pop('adcSamplesChan_%s' % str(chan), None)
        #     self.dataDict.pop('tdcSamplesChan_%s' % str(chan), None)


class MonitoringFigure:
    """Class to create figure and subplots based on user input"""

    # define figure and plot attributes
    def __init__(self, rows, cols):
        self.numRows = rows
        self.numCols = cols
        self.fig, self.axs = plt.subplots(self.numRows, self.numCols)
        self.fig.set_size_inches(18.5, 10.5, forward=True)

    def get_num_rows(self):
        return self.numRows

    def get_num_cols(self):
        return self.numCols

    def get_fig_obj(self):
        return self.fig

    def get_axs_obj(self):
        return self.axs


class MonitoringPlots:
    """Class to plot streaming ADC vs. TDC data"""

    # instance variables unique to each instance
    def __init__(self, dataDict, thresh, rows, cols, fig, axs):
        # figure parameters
        self.numRows = rows
        self.numCols = cols
        self.fig = fig
        self.axs = axs
        # lists for colors and markers
        self.cl = ['tab:blue', 'tab:orange', 'tab:green', 'tab:red', 'tab:purple',
                   'tab:brown', 'tab:pink', 'tab:gray', 'tab:olive', 'tab:cyan']
        self.ml = ['o', '^', 's', 'p', 'P', '*', 'X', 'd']
        # random channel list, ascending and non-repeating
        self.rcl = random.sample(range(1, numChans + 1), self.numRows * self.numCols)
        self.rcl.sort()
        # event number list, hit threshold (adc channels)
        self.enl = []
        self.hitThresh = thresh
        self.ic = 0
        for row in range(self.numRows):
            for column in range(self.numCols):
                self.axs[row, column].cla()
                self.axs[row, column].plot(dataDict['tdcSamplesChan_%d' % self.rcl[self.ic]],
                                           dataDict['adcSamplesChan_%d' % self.rcl[self.ic]],
                                           color=self.cl[self.ic % len(self.cl)],
                                           marker=self.ml[self.ic % len(self.ml)],
                                           ls='', label='Channel %d' % self.rcl[self.ic])
                hitLoc = np.where(dataDict['adcSamplesChan_%d' % self.rcl[self.ic]] > 100)[0] + \
                         np.min(dataDict['tdcSamplesChan_%d' % self.rcl[self.ic]])
                if len(hitLoc) != 0: self.axs[row, column].set_xlim(np.min(hitLoc) - 10, np.max(hitLoc) + 20)
                self.axs[row, column].set_ylim(0, 1024)
                if column == 0:
                    self.axs[row, column].set_ylabel('ADC Value')
                if row == self.numRows - 1:
                    self.axs[row, column].set_xlabel('TDC Sample Number')
                self.axs[row, column].legend(loc='best', markerscale=0, handletextpad=0, handlelength=0)
                self.ic += 1
        plt.tight_layout()
        # plt.savefig('plots/event_%d.png' % ec)
        plt.pause(0.05)
        # remove event data after being published
        for chan in range(1, numChans + 1):
            dataDict.pop('adcSamplesChan_%s' % str(chan), None)
            dataDict.pop('tdcSamplesChan_%s' % str(chan), None)


# instantiate the monitoring figure class
monFig = MonitoringFigure(2, 2)
# receive zmq messages from jana publisher
while True:
    # receive zmq packets from jana publisher
    janaMessage = subscriber.recv()
    # instantiate the indra message class
    jevent = IndraMessage(janaMessage)
    # jevent = IndraMessage(janaMessage, publisher)
    # instantiate the monitoring plot class
    monPlots = MonitoringPlots(jevent.dataDict, 100,
                               monFig.get_num_rows(), monFig.get_num_cols(),
                               monFig.get_fig_obj(), monFig.get_axs_obj())

# We never get here but clean up anyhow
# subscriber.close()
# subContext.term()
# publisher.close()
# pubContext.term()
