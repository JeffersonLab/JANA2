#!/bin/tcsh -f

#
# This is an example script for running a job multiple
# times, each with a different number of threads. It
# uses /usr/bin/time to run it so that the memory usage
# can be extracted. Each job directs output to a separate
# text file. 
# 
# Once all jobs complete, run this command to put a summary
# of the memory usage into mem.dat
#
# grep Maximum *.out | sed -e 's/Maximum resident set size (kbytes)://g' | sed -e 's/threads\.out://g' | sort -n > mem.dat
#
# Finally, use the RAM_vs_nthreads.C ROOT macro to make
# a pretty plot.
#

setenv FILENAME       hd_rawdata_002931_002.evio
setenv PLUGINS        danarest
setenv EVENTS_TO_KEEP 10000


foreach t (1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32)
  rm -f dana_rest.hddm
  /usr/bin/time -v \
     hd_ana \
	-PNTHREADS=${t} \
  	-PPLUGINS=${PLUGINS} \
	-PEVENTS_TO_KEEP=${EVENTS_TO_KEEP} \
	${FILENAME} |& tee ${t}threads.out
end

