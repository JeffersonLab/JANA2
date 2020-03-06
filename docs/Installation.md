---
title: JANA: Multi-threaded HENP Event Reconstruction
---

<center>
<table border="0" width="100%" align="center">
<TH width="20%"><A href="index.html">Welcome</A></TH>
<TH width="20%"><A href="Tutorial.html">Tutorial</A></TH>
<TH width="20%"><A href="Howto.html">How-to guides</A></TH>
<TH width="20%"><A href="Explanation.html">Principles</A></TH>
<TH width="20%"><A href="Reference.html">Reference</A></TH>
</table>
</center>


### Building JANA

First, set your `$JANA_HOME` environment variable. This is where the executables, libraries, headers, and plugins
get installed. You will need to tell CMake to install to `$JANA_HOME` manually -- this is on purpose to avoid accidentally
clobbering an existing JANA installation. Luckily, all you have to do is pass `-DCMAKE_INSTALL_PREFIX=$JANA_HOME`
to CMake. Be aware that if you change $JANA_HOME, you'll need to rerun cmake, and sometimes you'll also need to invalidate your CMake cache. 
Also be aware that although CMake usually defaults `CMAKE_INSTALL_PREFIX` to `/usr/local`, we have disabled this because 
we rarely want this in practice, and we don't want the build system picking up outdated headers and libraries we installed to `/usr/local` by accident. 
If you want to set `JANA_HOME=/usr/local`, you are free to do so, but you must do so deliberately.

Next, set your build directory. This is where CMake's caches, logs, intermediate build artifacts, etc go. The convention
is to name it `build` and put it in the project's root directory. If you are using CLion, it will automatically create 
a `cmake-build-debug` directory which works just fine. 

Finally, you can cd into your build directory and build and install everything the usual CMake way.

~~~ bash
git clone https://github.com/JeffersonLab/JANA2   # Get JANA
export JANA_HOME=~/jana_home                      # Set install dir
export PATH=$PATH:$JANA_HOME/bin                  # Put jana executable on path

mkdir build                                       # Set build dir
cd build
cmake ../JANA2 -DCMAKE_INSTALL_PREFIX=$JANA_HOME  # Generate makefiles
make -j8 install                                  # Build (using 8 threads) and install
jana -Pplugins=JTest                              # Run JTest plugin to verify successful install

~~~

By default, JANA will look for plugins under `$JANA_HOME/plugins`. For your plugins to propagate here, you have to `install`
them. If you don't want to do that, you can also set the environment variable `$JANA_PLUGIN_PATH` to point to the build
directory of your project. JANA will report where exactly it went looking for your plugins and what happened when it tried
to load t them if you set the JANA config `jana:debug_plugin_loading=1`.

~~~ bash
jana -Pplugins=JTest -Pjana:debug_plugin_loading=1
~~~


### Running JANA

JANA is typically run like this:

~~~ bash
$JANA_HOME/bin/jana -Pplugins=JTest -Pnthreads=8 ~/data/inputfile.txt
~~~

Note that the JANA executable won't do anything until you provide plugins.
A simple plugin is provided called JTest, which verifies that everything is working and optionally does a quick
performance benchmark. Additional simple plugins are provided in `src/examples`. Instructions on how to write your
own are given in the Tutorial section.

Along with specifying plugins, you need to specify the input files containing the events you wish to process.
Note that JTest ignores these and crunches randomly generated data instead.


The command-line flags are:

| Short | Long | Meaning  |
|:------|:-----|:---------|
| -h    | --help               | Display help message |
| -v    | --version            | Display version information |
| -c    | --configs            | Display configuration parameters |
| -l    | --loadconfigs <file> | Load configuration parameters from file |
| -d    | --dumpconfigs <file> | Dump configuration parameters to file |
| -b    | --benchmark          | Run JANA in benchmark mode |
| -P    |                      | Specify a configuration parameter (see below) |



The most commonly used configuration parameters are below. Your plugins can define their own parameters
and automatically get them from the command line or config file as well.


| Name | Description |
|:-----|:------------|
plugins                   | Comma-separated list of plugin filenames. JANA will look for these on the `$JANA_PLUGIN_PATH`
nthreads                  | Size of thread team (Defaults to the number of cores on your machine)
jana:debug_plugin_loading | Log additional information in case JANA isn't finding your plugins
jtest:nsamples            | Number of randomly-generated events to process when running JTEST

