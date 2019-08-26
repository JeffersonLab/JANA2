#!/usr/bin/python3.6

import numpy as np
import uproot as ur
import matplotlib.pyplot as plt
import random, os, time

# end of stream flag, root input file
eos = 0; rif = 'outFile.root'
# initialize counters
sizeDiffCntr = 0; fileEndCntr = 0
# number of channels to analyze
numChans = 80

# plot adc samples vs. tdc samples
numRows = 6; numCols = 6
fig, axs = plt.subplots(numRows, numCols)
fig.set_size_inches(18.5, 10.5, forward = True)

# tfig = plt.figure()
# tfig.set_size_inches(18.5, 10.5, forward = True)
# gs = tfig.add_gridspec(numRows, numCols)

# lists for colors and markers
cl = ['tab:blue',  'tab:orange', 'tab:green', 'tab:red',   'tab:purple',
      'tab:brown', 'tab:pink',   'tab:gray',  'tab:olive', 'tab:cyan']
ml = ['o', '^', 's', 'p', 'P', '*', 'X', 'd']

# random channel list, ascending and non-repeating
rcl = random.sample(range(1, numChans + 1), numRows*numCols)
rcl.sort()

# event number list, hit threshold (adc channels), initialize occupancy array
enl = []; hitThresh = 100
detOcc = np.zeros(numChans)

# fit function
def hitFunc(sample, peak, startTime, decayTime, baseLine):
    adcSample = np.piecewise(sample, [sample < startTime, sample >= startTime], [lambda sample: baseLine,
                             lambda sample: (peak * np.power(((sample - startTime) / decayTime), 4) *
                                             np.exp((-4) * ((sample - startTime) / decayTime)) + baseLine)])
    return adcSample

while eos == 0 :
    rifInitSize = os.stat(rif).st_size
    time.sleep(1)
    rifCurrSize = os.stat(rif).st_size
    print('rifInitSize = %d, rifCurrSize = %d, fileEndCntr = %d' % (rifInitSize, rifCurrSize, fileEndCntr))
    sizeDiff = rifCurrSize - rifInitSize
    if sizeDiff > 0 :
        sizeDiffCntr += 1
        print('sizeDiff = %d, sizeDiffCntr = %d' % (sizeDiff, sizeDiffCntr))
        # uproot the event tree
        et = ur.open(rif)['ET']
        # handle event number tracking
        enl.append(et[b'event'].numentries)
        currEvent = enl[len(enl) - 1]
        if len(enl) == 1 : prevEvent = 1
        else : prevEvent = enl[len(enl) - 2]
        numEventsBetween = currEvent - prevEvent
        print ('enl = ', enl)
        print ('currEvent = %d, prevEvent = %d, numEventsBetween = %d' % (currEvent, prevEvent, numEventsBetween))
        # uproot the sample tree
        st = ur.open(rif)['ST']
        # populate the data dictionaries
        adcDict = st.arrays(['adc*'])
        tdcDict = st.arrays(['tdc*'])
        print('eventNum = %d, size of adcDict[b\'adcSamplesChan_80\'] = %d' % (enl[len(enl) - 1], len(adcDict[b'adcSamplesChan_80'])))
        # update the detector occupancy array
        for chan in range(1, numChans + 1) :
            for ievent in range(1, numEventsBetween + 1) :
                for sample in adcDict[b'adcSamplesChan_%d' % chan][1024*(prevEvent + ievent - 1):1024*(prevEvent + ievent)] :
                    if sample > hitThresh :
                        detOcc[chan-1] += 1
                        break
            if chan % 10 == 0 : print('decOcc for chan %d = ' % chan, detOcc[chan-1])
        ic = 0  # index counter
        for row in range(numRows) :
            for column in range(numCols) :
                axs[row, column].cla()
                axs[row, column].plot(tdcDict[b'tdcSamplesChan_%d' % rcl[ic]][-1024:],
                                      adcDict[b'adcSamplesChan_%d' % rcl[ic]][-1024:],
                                      color = cl[ic % len(cl)], marker = ml[ic % len(ml)],
                                      ls = '', label = 'Channel %d' % rcl[ic])
                hitLoc = np.where(adcDict[b'adcSamplesChan_%d' % rcl[ic]][-1024:]>100)[0] + len(adcDict[b'adcSamplesChan_%d' % rcl[ic]]) - 1024
                if len(hitLoc) != 0 : axs[row, column].set_xlim(np.min(hitLoc) - 10, np.max(hitLoc) + 20)
                axs[row, column].set_ylim(0, 1024)
                if column == 0 : axs[row, column].set_ylabel('ADC Value')
                if row == numRows - 1 : axs[row, column].set_xlabel('TDC Sample Number')
                axs[row, column].legend(loc = 'best', markerscale = 0, handletextpad = 0, handlelength = 0)
                ic += 1
        plt.tight_layout()
        plt.pause(0.05)
    else :
        fileEndCntr += 1
    if fileEndCntr == 60 :
        print('\nData stream no longer present!  Exiting now...buh bye!\n')
        eos = 1