#!/bin/bash

echo "Mounting CVMFS"
yum -y install fuse
chmod 666 /dev/fuse
mkdir -p /cvmfs/oasis.opensciencegrid.org
mount -t cvmfs oasis.opensciencegrid.org /cvmfs/oasis.opensciencegrid.org
ln -s /cvmfs/oasis.opensciencegrid.org/gluex/group /group

export BMS_OSNAME_OVERRIDE="Linux_Alma9-x86_64-gcc11.4.1-cntr"
# The BMS_OSNAME override is needed because Alma9 retroactively switched its system gcc
# from 11.4.1 to 11.5. We cannot create a new Alma9 container that uses gcc11.4.1, but
# the CVMFS artifact repository hasn't been updated to reflect that.

source /group/halld/Software/build_scripts/gluex_env_boot_jlab.sh
gxenv /workspace/JANA2/.github/halld_recon_build_prereqs_version.xml

echo "ROOTSYS=$ROOTSYS"
cd /workspace/halld_recon/src
scons install -j12 DEBUG=1 OPTIMIZATION=0 SHOWBUILD=1

