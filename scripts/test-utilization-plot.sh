#!/bin/bash

set -e # Exit on fail
set -x # Print each command

jana -Pplugins=JTest \
    -Pnthreads=Ncores \
    -Pjana:nevents=200 \
    -Pjana:log:show_threadstamp=1 \
    -Pjana:loglevel=debug \
    -Pjtest:plotter:cputime_ms=40 \
    -Pjtest:use_legacy_plotter=0 >log_new.txt

jana -Pplugins=JTest \
    -Pnthreads=Ncores \
    -Pjana:nevents=200 \
    -Pjana:log:show_threadstamp=1 \
    -Pjana:loglevel=debug \
    -Pjtest:plotter:cputime_ms=40 \
    -Pjtest:use_legacy_plotter=1 >log_old.txt

jana-plot-utilization.py log_old.txt timeline_old.svg
jana-plot-utilization.py log_new.txt timeline_new.svg
