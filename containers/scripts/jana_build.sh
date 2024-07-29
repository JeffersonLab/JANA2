#!/bin/bash

export CC=$(which gcc)
export CXX=$(which g++)
export BUILD_SCRIPTS=/group/halld/Software/build_scripts
export JANA_HOME=/JANA2
export JANA_PLUGIN_PATH=$JANA_HOME/plugins

source $BUILD_SCRIPTS/gluex_env_boot_jlab.sh --bs $BUILD_SCRIPTS
gxenv $JANA_HOME/containers/prereqs_versions/jana_prereqs_version.xml

echo "jana_home value: $JANA_HOME"

cd $JANA_HOME
mkdir -p build
cd build

echo "Building start"
cmake3 $JANA_HOME -DUSE_XERCES=1
make -j8 install
