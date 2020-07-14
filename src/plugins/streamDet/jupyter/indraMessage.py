#!/usr/bin/python3.6

# Copyright 2020, Jefferson Science Associates, LLC.
# Subject to the terms in the LICENSE file found in the top-level directory.

import struct
import numpy as np

# define global variables
numChans = 80


class IndraMessage:
    """Class to handle decoding of ZMQ INDRA Messages"""

    def __init__(self, zmq_msg):
        # def __init__(self, zmq_msg, zmq_pub):
        # define the data dictionary
        self.data_dict = {}
        # size up the zmq message
        self.msg_size = len(zmq_msg)
        # indra message header is a fixed 56 bytes
        self.payload_bytes = self.msg_size - 56
        # unpack the message according to the indra message protocol
        self.msg = struct.unpack('IIIIIIIQqq%ds' % self.payload_bytes, zmq_msg)
        # decode the indra message members
        self.source_id = self.msg[0]
        self.total_bytes = self.msg[1]
        self.payload_bytes = self.msg[2]
        self.compressed_bytes = self.msg[3]
        self.magic = self.msg[4]
        self.format_vrsn = self.msg[5]
        self.flags = self.msg[6]
        self.record_cntr = self.msg[7]
        self.time_stamps_sec = self.msg[8]
        self.time_stamps_nsec = self.msg[9]
        self.payload = self.msg[10]
        # print message info
        print('INDRA Message received -> event = %d, size = %d bytes' % (self.record_cntr, self.msg_size))
        # adc samples are uints of length 4 plus 1 space delimiter, convert to array of ints
        self.adc_smpls_str = np.frombuffer(self.payload, dtype='S5')
        self.adc_smpls = np.reshape(self.adc_smpls_str.astype(np.int), (1024, 80))
        # define data dictionary keys and data types
        for chan in range(1, numChans + 1):
            self.data_dict.update({'adcSamplesChan_%s' % chan: np.array([])})
            self.data_dict.update(
                {'tdcSamplesChan_%s' % chan: np.arange((self.record_cntr - 1) * 1024, self.record_cntr * 1024)})
        # enumerate the samples object and populate the data dictionary
        for index, sample in np.ndenumerate(self.adc_smpls):
            self.data_dict['adcSamplesChan_%s' % str(index[1] + 1)] = \
                np.append(self.data_dict['adcSamplesChan_%s' % str(index[1] + 1)], sample)
        # serialize the data dictionary via pickle and publish it
        # self.event_data_dict = pickle.dumps(self.data_dict)
        # zmq_pub.send_pyobj(self.event_data_dict)
        # remove event data after being published
        # for chan in range(1, numChans + 1) :
        #     self.data_dict.pop('adcSamplesChan_%s' % str(chan), None)
        #     self.data_dict.pop('tdcSamplesChan_%s' % str(chan), None)
