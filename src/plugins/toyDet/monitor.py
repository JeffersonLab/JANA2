#!/usr/bin/python3.6

import zmq, sys
import struct

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

while True:
	# acquire zmq packets from jana publisher
	janaZmqData = subscriber.recv()
	janaZmqDataSize = sys.getsizeof(janaZmqData)
	print('janaZmqDataSize = %d, janaZmqData = %s, len = %d' % (janaZmqDataSize, janaZmqData, len(janaZmqData)))
	payload_bytes = len(janaZmqData) - 56
	indra_message = struct.unpack('IIIIIIIQqq%ds' % payload_bytes, janaZmqData)
	print((indra_message[7], indra_message[0]))

# We never get here but clean up anyhow
subscriber.close()
context.term()
# publisher.close()
