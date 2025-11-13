#!/bin/bash

echo "Mounting CVMFS"
yum -y install fuse
chmod 666 /dev/fuse
mkdir -p /cvmfs/oasis.opensciencegrid.org
mount -t cvmfs oasis.opensciencegrid.org /cvmfs/oasis.opensciencegrid.org
ln -s /cvmfs/oasis.opensciencegrid.org/gluex/group /group

source /group/halld/Software/build_scripts/gluex_env_boot_jlab.sh
gxenv /workspace/JANA2/.github/halld_recon_build_prereqs_version.xml

echo "ROOTSYS=$ROOTSYS"
cd /workspace/halld_recon/src
scons install -j12 DEBUG=1 OPTIMIZATION=0 SHOWBUILD=1
