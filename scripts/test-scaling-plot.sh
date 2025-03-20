#!/bin/bash

set -e # Exit on fail
set -x # Print each command

jana -b \
    -Pplugins=JTest \
    -Pbenchmark:resultsdir=test1 \
    -Pbenchmark:nsamples=4 \
    -Pbenchmark:copyscript=false \
    -Pjtest:plotter:cputime_ms=5

jana -b \
    -Pplugins=JTest \
    -Pbenchmark:resultsdir=test2 \
    -Pbenchmark:nsamples=4 \
    -Pbenchmark:copyscript=false \
    -Pjtest:plotter:cputime_ms=10

jana -b \
    -Pplugins=JTest \
    -Pbenchmark:resultsdir=test3 \
    -Pbenchmark:nsamples=4 \
    -Pbenchmark:copyscript=false \
    -Pjtest:plotter:cputime_ms=20

echo "title = \"JANA2 Scaling Test: JTest\"" >>plotspec.toml
echo "output_filename = \"scalingplot.pdf\"" >>plotspec.toml
# echo "use_latex = true" >>plotspec.toml

echo >>plotspec.toml

echo "[[test]]" >>plotspec.toml
echo "key = \"t_seq = 5 ms\"" >>plotspec.toml
echo "datafile = \"test1/rates.dat\"" >>plotspec.toml
echo "t_seq = 5" >>plotspec.toml
echo "t_par = 145" >>plotspec.toml

echo >>plotspec.toml

echo "[[test]]" >>plotspec.toml
echo "key = \"t_seq = 10 ms\"" >>plotspec.toml
echo "datafile = \"test2/rates.dat\"" >>plotspec.toml
echo "t_seq = 10" >>plotspec.toml
echo "t_par = 145" >>plotspec.toml

echo >>plotspec.toml

echo "[[test]]" >>plotspec.toml
echo "key = \"t_seq = 20 ms\"" >>plotspec.toml
echo "datafile = \"test3/rates.dat\"" >>plotspec.toml
echo "t_seq = 20" >>plotspec.toml
echo "t_par = 145" >>plotspec.toml

jana-plot-scaletests.py plotspec.toml
