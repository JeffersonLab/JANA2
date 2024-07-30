#!/bin/bash

cd /workspace/halld_recon

export BUILD_SCRIPTS=/group/halld/Software/build_scripts

source $BUILD_SCRIPTS/gluex_env_boot_jlab.sh --bs $BUILD_SCRIPTS
gxenv /workspace/JANA2/.github/halld_recon_build_prereqs_version.xml

chmod +x $JANA_HOME/bin/*

cd src
nice scons install
