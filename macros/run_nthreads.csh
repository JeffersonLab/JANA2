#!/bin/tcsh -f 

#
# This is an example script for running a job multiple
# times, each with a different number of threads. By
# using the janarate plugin, a ROOT TTree is created 
# with entries containing performance info throughout
# the job. Once the jobs are all finished the ROOT
# macro "rate_vs_nthreads.C" can be used to make a plot.
# 
# Note that is is customized for JLab Hall-D since it
# uses the hd_root program and the DDetectorMatches 
# factory (and the hd_rawdata_002931_002.evio file).
#
# The PRESCALE value is used to prescale how often an
# entry in the tree is made. By not making an entry
# for every event, the program is less affected by the
# overhead of making the tree entries.
#

setenv FILENAME       hd_rawdata_002931_002.evio
setenv PLUGINS        janarate
setenv PRESCALE       100
setenv EVENTS_TO_KEEP 100000
setenv FACTORIES      DDetectorMatches

foreach t (1 2 4 8 12 16 20 24 28 32 36)
  hd_root -o rate${t}.root \
	-PNTHREADS=${t} \
  	-PPLUGINS=${PLUGINS} \
	-PRATE:PRESCALE=${PRESCALE}
	-PEVENTS_TO_KEEP=${EVENTS_TO_KEEP} \
	-PAUTOACTIVATE=${FACTORIES} \
	${FILENAME}
end
