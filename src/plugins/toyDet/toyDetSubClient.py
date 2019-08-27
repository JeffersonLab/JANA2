#!/usr/bin/python3.6

import zmq, struct
import numpy as np

subPort = 5557
pubPort = 5558

context    = zmq.Context()
subscriber = context.socket(zmq.SUB)
subscriber.setsockopt(zmq.SUBSCRIBE, b'')
subscriber.connect('tcp://localhost:%d' % subPort)

# publisher = context.socket(zmq.PUSH)
# publisher.connect('tcp://localhost:%d' % pubPort)
# publisher.send(b'')

print('\nSubscribing to tcp://localhost:%d' % subPort)
# print('Publishing on tcp://localhost:%d\n'  % pubPort)

while True :
    # acquire zmq packets from jana publisher
    jzmqPacket = subscriber.recv()
    # print('jzmqPacket = %s', jzmqPacket)
    jzmqPacketSize   = len(jzmqPacket)
    jzmqPayloadBytes = jzmqPacketSize - 56
    print('jzmqPacketSize = %d, jzmqPayloadBytes = %d' % (jzmqPacketSize, jzmqPayloadBytes))
    jzmqIndraMessage = struct.unpack('IIIIIIIQqq%ds' % jzmqPayloadBytes, jzmqPacket)
    print('IM source_id = %d, IM total_bytes = %d, IM payload_bytes = %d, IM record_counter = %d' % (jzmqIndraMessage[0], jzmqIndraMessage[1], jzmqIndraMessage[2], jzmqIndraMessage[7]))

    jeventNum = jzmqIndraMessage[7]

    adcData.append(-jzmqPayloadBytes:)




# We never get here but clean up anyhow
subscriber.close()
context.term()
# publisher.close()