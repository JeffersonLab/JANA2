#!/usr/bin/python3.6

# Copyright 2020, Jefferson Science Associates, LLC.
# Subject to the terms in the LICENSE file found in the top-level directory.

import argparse
import numpy as np
import matplotlib.pyplot as plt
from scipy.stats import gumbel_r
import pandas as pd

parser = argparse.ArgumentParser()
parser.add_argument('-sr', '--sampleRate', metavar='SAMPLERATE', type=int, nargs=1, required=True,
                    help='sampling rate in MHz of SAMPA chips -> [5], [10], or [20]')
parser.add_argument('-nc', '--numChans', metavar='NUMCHANS', type=int, nargs=1, required=True,
                    help='number of channels to simulate')
parser.add_argument('-ne', '--numEvents', metavar='NUMEVENTS', type=int, nargs=1, required=True,
                    help='number of events to simulate')
parser.add_argument('-spec', '--spectra', metavar='SPECTRA', type=str, nargs=1, required=True,
                    help='ADC spectra to simulate -> [gumbel] or [sampa]')
parser.add_argument('-m', '--mode', metavar='MODE', type=str, nargs=1,
                    help='SAMPA DAQ mode to simulate -> [das] or [dsp]')
args = parser.parse_args()

sampleRate = args.sampleRate[0]
numChans = args.numChans[0]
numEvents = args.numEvents[0]
specMode = args.spectra[0]
if args.mode : adcMode = args.mode[0]

def sampaHitFunc(sample, peak, startTime, decayTime, baseLine):
    adcSample = np.piecewise(sample, [sample < startTime, sample >= startTime], [lambda sample: baseLine,
                             lambda sample: (peak * np.power(((sample - startTime) / decayTime), 4) *
                                             np.exp((-4) * ((sample - startTime) / decayTime)) + baseLine)])
    return adcSample

class DataStream:

    def __init__(self, sampleRate, numEvents):
        # sampling rate (s^-1), number of events to simulate
        self.sampleRate, self.numEvents = sampleRate, numEvents
        # number of samples in read-out window, up-to 1024 samples for SAMPA chips
        self.numSamples = 1024
        # read-out window length (s)
        self.windowWidth = (1.0 / self.sampleRate) * self.numSamples
        # length of run (s)
        self.runTime = self.numEvents * self.windowWidth
        # initial time sample
        self.initTime = 0.0

    # method to simulate time samples
    def simTimeSamples(self, windowStart, windowEnd):
        self.timeSamples = np.linspace(windowStart, windowEnd, int(self.numSamples))
        self.initTime = self.timeSamples[-1]
        self.randTimeIndex = np.random.randint(int(len(self.timeSamples) * 0.25),
                                               high=int(len(self.timeSamples) * 0.75))

    # method to simulate arbitrary adc samples via a gumbel distribution
    # def simAdcSignals(self, numSamples, randTimeIndex) :
    def gumbelAdcSignals(self):
        self.baseLine = np.random.rand(int(self.numSamples)) * 1.0e-2
        self.hitJudge = np.random.random()
        if (self.hitJudge < 0.5):  # simulate no hit
            self.adcSamples = self.baseLine
        elif (self.hitJudge >= 0.5):  # simulate hit
            self.gumbelMean = np.random.random()
            self.gumbelBeta = np.random.random()
            self.gumbelPpf = np.linspace(gumbel_r.ppf(0.001, loc=self.gumbelMean, scale=self.gumbelBeta),
                                         gumbel_r.ppf(0.999, loc=self.gumbelMean, scale=self.gumbelBeta), 100)
            self.gumbelPdf = gumbel_r.pdf(self.gumbelPpf, loc=self.gumbelMean, scale=self.gumbelBeta)
            self.adcSamples = np.insert(self.baseLine, self.randTimeIndex, self.gumbelPdf)[:int(self.numSamples)]

    # method to simulate sampa adc samples via piecewise fourth order semi-gaussian
    def sampaAdcSignals(self):
        self.baseLine = np.random.randint(60, 81, size=self.numSamples)
        self.hitJudge = np.random.randint(0, 2, size=1)[0]
        if (self.hitJudge == 0):  # simulate no hit
            self.adcSamples = self.baseLine
        elif (self.hitJudge == 1):  # simulate hit
            self.peak = np.random.randint(5000, 50001, size=1)[0]
            self.baseLine = np.random.randint(60, 81, size=self.numSamples)
            self.startTime = np.random.randint(4, 7, size=1)[0]
            self.decayTime = np.random.randint(2, 5, size=1)[0]
            self.numHitSamples = np.random.randint(10, 16, size=1)[0]
            self.samples = np.arange(0, self.numHitSamples + 11)
            self.hitSamples = sampaHitFunc(self.samples, self.peak, self.startTime, self.decayTime, np.average(self.baseLine))
            self.adcSamples = np.insert(self.baseLine, self.randTimeIndex, self.hitSamples)[:int(self.numSamples)]

    def __iter__(self):
        return self

    def __next__(self):
        if (self.initTime == 0.0):
            # simulate time and adc samples
            self.simTimeSamples(self.initTime, self.windowWidth)
            if specMode == 'gumbel': self.gumbelAdcSignals()
            if specMode == 'sampa': self.sampaAdcSignals()
        elif (self.initTime > 0.0 and self.initTime < self.runTime):
            # simulate the time and adc signals
            self.simTimeSamples(self.initTime, self.initTime + self.windowWidth)
            if specMode == 'gumbel': self.gumbelAdcSignals()
            if specMode == 'sampa': self.sampaAdcSignals()
        elif (self.initTime >= self.runTime):
            raise StopIteration

# # data file for streaming analysis
# datFile = open('run-%2.0d-mhz-%d-chan-%d-ev.dat' % (sampleRate, numChans, numEvents), 'w+')
# for chan in range(1, numChans+1) : 
#     datFile.write('# channel = %d\n' % chan)
#     dataObj = DataStream(sampleRate*1.0e+6, numEvents)
#     for event in dataObj : 
#         np.savetxt(datFile, (dataObj.timeSamples, dataObj.adcSamples), fmt='%.9f')
#    # plt.plot(dataObj.timeSamples, dataObj.adcSamples)
#    # plt.xlabel('TDC Sample (arb. units)')
#    # plt.ylabel('ADC Sample (arb. units)')
#    # plt.title('Channel %d' % chan)
#    # plt.show()
# datFile.close()

# gumbel data file for event based analysis
if specMode == 'gumbel' :
    datFile = open('run-%d-mhz-%d-chan-%d-ev.dat' % (sampleRate, numChans, numEvents), 'w+')
    for ievent in range(1, numEvents + 1):
        datFile.write('@ event = %d\n' % ievent)
        for chan in range(1, numChans + 1):
            datFile.write('# channel = %d\n' % chan)
            dataObj = DataStream(sampleRate * 1.0e+6, 1)
            for event in dataObj:
                if len(dataObj.timeSamples) != len(dataObj.adcSamples): print("!!!Something terrible is amiss!!!")
                np.savetxt(datFile, (np.add(dataObj.timeSamples, dataObj.windowWidth * (ievent - 1)), dataObj.adcSamples), fmt='%.9f')
    # plt.plot(dataObj.timeSamples, dataObj.adcSamples)
    # plt.xlabel('TDC Sample (arb. units)')
    # plt.ylabel('ADC Sample (arb. units)')
    # plt.title('Channel %d' % chan)
    # plt.show()
    datFile.close()

if specMode == 'sampa' :
    datFile = open('run-%d-mhz-%d-chan-%d-ev.dat' % (sampleRate, numChans, numEvents), 'w+')
    dfl = []; esl = []  # data frame list, event series list
    for ievent in range(1, numEvents + 1):
        csl = []  # channel series list
        for chan in range(1, numChans + 1):
            dataObj = DataStream(sampleRate * 1.0e+6, 1)
            for event in dataObj:
                if len(dataObj.timeSamples) != len(dataObj.adcSamples): print("!!!Something terrible is amiss!!!")
                csl.append(pd.Series(dataObj.adcSamples, name = 'chan_%d' % chan))
        esl.append(csl)
        dfl.append(pd.concat(esl[ievent-1], axis = 1))
    df = pd.concat(dfl, ignore_index=True)
    np.savetxt(datFile, df.values, fmt='%04d')
    # df.plot(y = 'chan_1', use_index = True, marker = 'o', c = 'tab:blue', ls = '')
    # plt.xlabel('TDC Sample (arb. units)')
    # plt.ylabel('ADC Sample (arb. units)')
    # plt.title('Channel %d' % chan)
    # plt.show()
    datFile.close()