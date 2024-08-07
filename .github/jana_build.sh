#!/bin/bash
echo "mounting cvmfs"
yum -y install fuse
chmod 666 /dev/fuse
mkdir -p /cvmfs/oasis.opensciencegrid.org
mount -t cvmfs oasis.opensciencegrid.org /cvmfs/oasis.opensciencegrid.org

export CC=$(which gcc)
export CXX=$(which g++)
export BUILD_SCRIPTS=/cvmfs/oasis.opensciencegrid.org/gluex/group/halld/Software/build_scripts
export PROJECT_ROOT=/workspace
export JANA_HOME=$PROJECT_ROOT/JANA2
export JANA_PLUGIN_PATH=$PROJECT_ROOT/JANA2/plugins

source $BUILD_SCRIPTS/gluex_env_boot_jlab.sh --bs $BUILD_SCRIPTS
gxenv $PROJECT_ROOT/JANA2/.github/jana_prereqs_version.xml

echo "jana_home value: $JANA_HOME"

cd $JANA_HOME
mkdir -p build
cd build

echo "Building start"
cmake3 $JANA_HOME -DUSE_XERCES=1 -DCMAKE_CXX_STANDARD=17
make install -j12
