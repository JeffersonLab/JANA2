#!/usr/bin/python3.6

import zmq, pickle, random
import numpy as np
import matplotlib.pyplot as plt

# define the subscriber port
subPort = 5558
# configure the subscriber
subContext = zmq.Context()
subscriber = subContext.socket(zmq.SUB)
subscriber.setsockopt(zmq.SUBSCRIBE, b'')
subscriber.connect('tcp://127.0.0.1:%d' % subPort)
print('\nSubscribing to tcp://127.0.0.1:%d\n' % subPort)

# plot adc samples vs. tdc samples
numRows = 6; numCols = 6; numChans = 80
fig, axs = plt.subplots(numRows, numCols)
fig.set_size_inches(18.5, 10.5, forward = True)

# lists for colors and markers
cl = ['tab:blue',  'tab:orange', 'tab:green', 'tab:red',   'tab:purple',
      'tab:brown', 'tab:pink',   'tab:gray',  'tab:olive', 'tab:cyan']
ml = ['o', '^', 's', 'p', 'P', '*', 'X', 'd']

# random channel list, ascending and non-repeating
rcl = random.sample(range(1, numChans + 1), numRows*numCols)
rcl.sort()

# event number list, hit threshold (adc channels), initialize occupancy array
enl = []; hitThresh = 100

ec = 0  # event counter
# recieve zmq messages from jana subscriber
while True :
    # receive zmq packets from jana subscriber
    picklePacket = subscriber.recv_pyobj()
    print('picklePacket received!')
    eventDataDict = pickle.loads(picklePacket)
    ec += 1
    ic = 0  # index counter
    for row in range(numRows) :
        for column in range(numCols) :
            axs[row, column].cla()
            axs[row, column].plot(eventDataDict['tdcSamplesChan_%d' % rcl[ic]],
                                  eventDataDict['adcSamplesChan_%d' % rcl[ic]],
                                  color = cl[ic % len(cl)], marker = ml[ic % len(ml)],
                                  ls = '', label = 'Channel %d' % rcl[ic])
            hitLoc = np.where(eventDataDict['adcSamplesChan_%d' % rcl[ic]]>100)[0] + np.min(eventDataDict['tdcSamplesChan_%d' % rcl[ic]])
            if len(hitLoc) != 0 : axs[row, column].set_xlim(np.min(hitLoc) - 10, np.max(hitLoc) + 20)
            axs[row, column].set_ylim(0, 1024)
            if column == 0 : axs[row, column].set_ylabel('ADC Value')
            if row == numRows - 1 : axs[row, column].set_xlabel('TDC Sample Number')
            axs[row, column].legend(loc = 'best', markerscale = 0, handletextpad = 0, handlelength = 0)
            ic += 1
    plt.tight_layout()
    # plt.savefig('plots/event_%d.png' % ec)
    plt.pause(0.05)