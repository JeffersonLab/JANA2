#!/usr/bin/python3.6

import zmq, struct, pickle
import numpy as np

# define the subscriber and publisher ports
subPort = 5557
pubPort = 5558
# define data dictionary
dataDict = {}
# define global variables
numChans = 80

# configure the subscriber
subContext = zmq.Context()
subscriber = subContext.socket(zmq.SUB)
subscriber.setsockopt(zmq.SUBSCRIBE, b'')
subscriber.connect('tcp://127.0.0.1:%d' % subPort)
print('\nSubscribing to JANA ZMQ Messages on socket tcp://127.0.0.1:%d\n' % subPort)
# configure the publisher
pubContext = zmq.Context()
publisher  = pubContext.socket(zmq.PUB)
publisher.bind('tcp://127.0.0.1:%d' % pubPort)
print('\nPublishing on tcp://127.0.0.1:%d\n'  % pubPort)

class IndraMessage:
    """Class to handle decoding of ZMQ INDRA Messages"""
    def __init__(self, zmqMessage):
        # size up the zmq message
        self.messageSize = len(zmqMessage)
        # indra message header is a fixed 56 bytes
        self.payloadBytes = self.messageSize - 56
        # unpack the message according to the indra message protocol
        self.message = struct.unpack('IIIIIIIQqq%ds' % self.payloadBytes, zmqMessage)
        # decode the indra message members
        self.sourceId         = self.message[0]
        self.totalBytes       = self.message[1]
        self.payloadBytes     = self.message[2]
        self.compressedBytes  = self.message[3]
        self.magic            = self.message[4]
        self.formatVersion    = self.message[5]
        self.flags            = self.message[6]
        self.recordCounter    = self.message[7]
        self.timeStampSec     = self.message[8]
        self.timeStampNanoSec = self.message[9]
        self.payload          = self.message[10]
        # adc samples are uints of length 4 plus 1 space delimiter, convert to array of ints
        self.adcSamplesStr = np.frombuffer(self.payload, dtype = 'S5')
        self.adcSamples = np.reshape(self.adcSamplesStr.astype(np.int), (1024, 80))
        # print message info
        print('INDRA Message recieved -> event = %d, size = %d bytes' % (self.recordCounter, self.messageSize))

# receive zmq messages from jana publisher
while True :
    # receive zmq packets from jana publisher
    janaMessage = subscriber.recv()
    # instantiate the indra message class
    jevent = IndraMessage(janaMessage)
    # define data dictionary keys and data types
    for chan in range(1, numChans + 1) :
        dataDict.update({'adcSamplesChan_%s' % chan : np.array([])})
        dataDict.update({'tdcSamplesChan_%s' % chan : np.arange((jevent.recordCounter - 1) * 1024, jevent.recordCounter * 1024)})
        # dataDict['tdcSamplesChan_%s' % chan] = np.arange((jeventRecordCounter - 1) * 1024, jeventRecordCounter*1024)
    # enumerate the samples object and populate the data dictionary
    for index, sample in np.ndenumerate(jevent.adcSamples) :
        dataDict['adcSamplesChan_%s' % str(index[1] + 1)] = np.append(dataDict['adcSamplesChan_%s' % str(index[1] + 1)], sample)
    # serialize the data dictionary via pickle and publish it
    eventDataDict = pickle.dumps(dataDict)
    publisher.send_pyobj(eventDataDict)
    # remove event data after being published
    for chan in range(1, numChans + 1) :
        dataDict.pop('adcSamplesChan_%s' % str(chan), None)
        dataDict.pop('tdcSamplesChan_%s' % str(chan), None)

# We never get here but clean up anyhow
subscriber.close()
subContext.term()
publisher.close()
pubContext.term()