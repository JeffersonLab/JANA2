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

## Getting Started With JANA

For now, it is best to start with a fresh checkout of master. If you are trying to compile JANA as part of EIC,
we recommend using [ejpm](https://gitlab.com/eic/ejpm) instead as it can reason about transitive dependencies
such XERCES and ROOT. Otherwise, do the following:

~~~ bash
git clone https://github.com/JeffersonLab/JANA2 ~/jana2
~~~

JANA can be built using either CMake or SCons. CMake is recommended mainly because the rest of the C++ world
appears to be converging on using CMake. This makes for better tooling, e.g. tighter IDE integration.

### Building with CMake

First, set your `$JANA_HOME` environment variable. This is where the executables, libraries, headers, and plugins
get installed. If you don't set it, CMake will default to a system install, usually `/usr/local` under Linux. 
Sometimes it is more convenient to pass `JANA_HOME` as a CMake variable instead of an environment variable; the 
CMakeLists.txt will look for the CMake variable first, then the environment variable, and finally just pick the default. 
Note that if you change `$JANA_HOME`, you may have to rebuild the entire CMake project before CMake notices the change. 

Next, set your build directory. This is where CMake's caches, logs, intermediate build artifacts, etc go. The convention
is to name it `build` and put it in the project's root directory. If you are using CLion, it will automatically create 
a `cmake-build-debug` directory which works just fine.

~~~ bash
git clone https://github.com/JeffersonLab/JANA2   # Get JANA
export JANA_HOME=~/jana_home                      # Set install dir
export PATH=$PATH:$JANA_HOME/bin                  # Put jana executable on path

mkdir build                                       # Set build dir
cd build
cmake ../JANA2                                    # Generate makefiles
make -j8 install                                  # Build and install
jana -Pplugins=JTest                              # Run JTest plugin to verify successful install

~~~

By default, JANA will look for plugins under `$JANA_HOME/plugins`. For your plugins to propagate here, you have to `install`
them. If you don't want to do that, you can also set the environment variable `$JANA_PLUGIN_PATH` to point to the build
directory of your project. JANA will report where exactly it went looking for your plugins and what happened when it tried
to load t them if you set the JANA config `jana:debug_plugin_loading=1`.

~~~ bash
jana -Pplugins=JTest -Pjana:debug_plugin_loading=1
~~~


### Building with SCons

To build, you can simply type `scons` from the project root. If you have a multicore
build machine you can speed up compilation with the `-j` flag. Debugging is easier if you set `OPTIMIZATION=0`.
If you run into trouble, you can inspect the exact compiler arguments by specifying `SHOWBUILD=1`.
The main build script lives in the file `SConstruct`, and most of the functions it calls live in `SBMS/sbms.py`.

~~~ bash
~/jana2$ scons install -j8 OPTIMIZATION=0
~~~

By default, `scons` will build to a different directory for different operating systems, architectures, and
compilers. For instance, if you are compiling on CentOS 7 using gcc, your build directory might
be `~/jana2/Linux_CentOS7-x86_64-gcc4.8.5/`


~~~ bash
export JANA_HOME = ~/jana2/Linux_CentOS7-x86_64-gcc4.8.5/
export PATH=$PATH:$JANA_HOME/bin
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

