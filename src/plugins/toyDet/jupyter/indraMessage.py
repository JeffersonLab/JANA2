#!/usr/bin/python3.6

import struct
import numpy as np

# define global variables
numChans = 80


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
