#!/usr/bin/python3.6

import zmq, struct, pickle
import numpy as np

# define the subscriber and publisher ports
subPort = 5557
pubPort = 5558
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

# define data dictionary
dataDict = {}

# define global variables
numChans = 80

# receive zmq messages from jana publisher
while True :
    # receive zmq packets from jana publisher
    janaPacket = subscriber.recv()
    # INDRA message buffer header is 56 bytes
    janaPacketSize   = len(janaPacket)
    janaPayloadBytes = janaPacketSize - 56
    print('janaPacketSize = %d, janaPayloadBytes = %d' % (janaPacketSize, janaPayloadBytes))
    # unpack the zmq packets according to the INDRA message protocol
    janaIndraMessage = struct.unpack('IIIIIIIQqq%ds' % janaPayloadBytes, janaPacket)
    print('IM source_id = %d, IM total_bytes = %d, IM payload_bytes = %d, IM record_counter = %d' % (janaIndraMessage[0], janaIndraMessage[1], janaIndraMessage[2], janaIndraMessage[7]))
    # obtain the INDRA message members
    jeventSourceId         = janaIndraMessage[0]
    jeventTotalBytes       = janaIndraMessage[1]
    jeventPayloadBytes     = janaIndraMessage[2]
    jeventCompressedBytes  = janaIndraMessage[3]
    jeventMagic            = janaIndraMessage[4]
    jeventFormatVersion    = janaIndraMessage[5]
    jeventFlags            = janaIndraMessage[6]
    jeventRecordCounter    = janaIndraMessage[7]
    jeventTimeStampSec     = janaIndraMessage[8]
    jeventTimeStampNanoSec = janaIndraMessage[9]
    jeventPayload          = janaIndraMessage[10]
    # adc samples are uints of length 4 plus 1 space delimiter, convert to array of ints
    jeventAdcSamplesStr = np.frombuffer(jeventPayload, dtype='S5')
    jeventAdcSamples    = np.reshape(jeventAdcSamplesStr.astype(np.int), (1024, 80))
    # define data dictionary keys and data types
    for chan in range(1, numChans + 1) :
        dataDict.update({'adcSamplesChan_%s' % chan : np.array([])})
        dataDict.update({'tdcSamplesChan_%s' % chan : np.arange((jeventRecordCounter - 1) * 1024, jeventRecordCounter * 1024)})
        # dataDict['tdcSamplesChan_%s' % chan] = np.arange((jeventRecordCounter - 1) * 1024, jeventRecordCounter*1024)
    # enumerate the samples object and populate the data dictionary
    for index, sample in np.ndenumerate(jeventAdcSamples) :
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