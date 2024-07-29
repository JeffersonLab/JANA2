#!/bin/bash

cd /halld_recon

export BUILD_SCRIPTS=/group/halld/Software/build_scripts

source $BUILD_SCRIPTS/gluex_env_boot_jlab.sh --bs $BUILD_SCRIPTS
gxenv /JANA2/containers/prereqs_versions/halld_recon_build_prereqs_version.xml

chmod +x $JANA_HOME/bin/jana-config

cd src
nice scons install -j32
