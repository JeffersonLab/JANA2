# Benchmarking JANA2  <!-- {docsify-ignore-all} -->


JANA includes a built-in facililty for benchmarking programs and plugins. It produces a scalability curve by repeatedly pausing execution, adding additional worker threads, resuming execution, and measuring the resulting throughput over fixed time intervals. There is an additional option to measure the scalability curves for a matrix of different affinity and locality strategies. This is useful when your hardware architecture has nonuniform memory access.

In case you don't have JANA code ready to benchmark yet, JANA provides a plugin called `JTest` which can simulate different workloads. `JTest`  runs a dummy algorithm on randomly generated data, using a user-specified event size and number of FLOPs (floating point operations) per _event_. This gives a rough estimate of your code's performance. If you don't know the number of FLOPs per event, you can still compare the performance of JANA on different hardware architectures just by using the default settings.

Here is how you do benchmarking with `JTest`:

```bash

# Obtain and build JANA, if you haven't already
git clone http://github.com/JeffersonLab/JANA2
cd JANA2
mkdir build
mkdir install
export JANA_HOME=`pwd`/install
cmake -S . -B build
cmake --build build -j 10 --target install
cd install/bin

# Run the benchmarking
./jana -b -Pplugins=JTest
# -b enables benchmarking
# -Pplugins=JTest pulls in the JTest plugin
# Additional configuration options are listed below


# Benchmarking may take awhile. You can terminate any time without 
# losing data by pressing Ctrl-C _once or twice_. If you press it three
# times or more, it will hard-exit and won't write the results file.


cd JANA_Test_Results
# Raw data CSV files are in `samples.dat`
# Average and RMS rates are in `rates.dat`

# Show the scalability curve in a matplotlib window
./jana-plot-scaletest.py


```


If you already have a JANA project you would like to benchmark, all you have to do is build and install it the way you usually would, and then run

```bash
jana -b -Pplugins=$MY_PLUGIN
# Or
my_jana_app -b

cd JANA_Test_Results
# Raw data CSV files are in `samples.dat`
# Average and RMS rates are in `rates.dat`

# Show the scalability curve in a matplotlib window
./jana-plot-scaletest.py

```


These are the relevant configuration parameters for `JTest`:


| Name                 | Units  | Default           | Description                                       |
|:-------------------- |:------ |:----------------- |:------------------------------------------------- |
| benchmark:nsamples   | int    | 15                | Number of measurements made for each thread count |
| benchmark:minthreads | int    | 1                 | Minimum thread count                              |
| benchmark:maxthreads | int    | ncores            | Maximum thread count                              |
| benchmark:threadstep | int    | 1                 | Thread count increment                            |
| benchmark:resultsdir | string | JANA_Test_Results | Directory name for benchmark test results         |
