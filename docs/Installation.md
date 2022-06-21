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
get installed. (It is also where we will clone the source). CMake will install to `$JANA_HOME` if it is set (it
will install to `${CMAKE_BINARY_DIR}/install` if not). Be aware that although CMake usually defaults
`CMAKE_INSTALL_PREFIX` to `/usr/local`, we have disabled this because we rarely want this in practice, and we
don't want the build system picking up outdated headers and libraries we installed to `/usr/local` by accident.
If you want to set `JANA_HOME=/usr/local`, you are free to do so, but you must do so deliberately.

Next, set your build directory. This is where CMake's caches, logs, intermediate build artifacts, etc go. The convention
is to name it `build` and put it in the project's root directory. If you are using CLion, it will automatically create 
a `cmake-build-debug` directory which works just fine. 

Finally, you can cd into your build directory and build and install everything the usual CMake way.

~~~ bash
export JANA_VERSION=v2.0.5                    # Convenient to set this once for specific release
export JANA_HOME=${PWD}/JANA${JANA_VERSION}   # Set full path to install dir

git clone https://github.com/JeffersonLab/JANA2 --branch ${JANA_VERSION} ${JANA_HOME}  # Get JANA2

mkdir build                                   # Set build dir
cd build
cmake3 ${JANA_HOME}  # Generate makefiles     # Generate makefiles
make -j8 install                              # Build (using 8 threads) and install

source ${JANA_HOME}/bin/jana-this.sh          # Set PATH (and other envars)
jana -Pplugins=JTest                          # Run JTest plugin to verify successful install
~~~

Note: If you want to use a compiler other than the default one on your system, it is not enough to modify your
`$PATH`, as CMake ignores this by design. You either need to set the `CXX` environment variable or the 
`CMAKE_CXX_COMPILER` CMake variable.

By default, JANA will look for plugins under `$JANA_HOME/plugins`. For your plugins to propagate here, you have to `install`
them. If you don't want to do that, you can also set the environment variable `$JANA_PLUGIN_PATH` to point to the build
directory of your project. JANA will report where exactly it went looking for your plugins and what happened when it tried
to load them if you set the JANA config `jana:debug_plugin_loading=1`.

~~~ bash
jana -Pplugins=JTest -Pjana:debug_plugin_loading=1
~~~

### Using JANA in a CMake project

To use JANA in a CMake project, simply add `$JANA_HOME/lib/cmake/JANA` to your `CMAKE_PREFIX_PATH`,
or alternatively, set the CMake variable `JANA_DIR=$JANA_HOME/lib/cmake/JANA`.

### Using JANA in a non-CMake project

To use JANA in a non-CMake project:
1. Source `$JANA_HOME/bin/jana-this.sh` to set the environment variables needed for JANA's dependencies
2. Use `$JANA_HOME/bin/jana-config --cflags` to obtain JANA's compiler flags
3. Use `$JANA_HOME/bin/jana_config --libs` to obtain JANA's linker flags

