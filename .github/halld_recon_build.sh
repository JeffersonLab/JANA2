#!/bin/bash

echo "mounting cvmfs"
yum -y install fuse
chmod 666 /dev/fuse
mkdir -p /cvmfs/oasis.opensciencegrid.org
mount -t cvmfs oasis.opensciencegrid.org /cvmfs/oasis.opensciencegrid.org

cd /workspace/halld_recon
ln -s /cvmfs/oasis.opensciencegrid.org/gluex/group /group
export BUILD_SCRIPTS=/group/halld/Software/build_scripts
source $BUILD_SCRIPTS/gluex_env_boot_jlab.sh --bs $BUILD_SCRIPTS
gxenv /workspace/JANA2/.github/halld_recon_build_prereqs_version.xml

echo "rootsys"
echo $ROOTSYS
chmod +x $JANA_HOME/bin/*
cd src
scons install -j12