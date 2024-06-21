#!/bin/bash

#SBATCH -N 1  # number of nodes
#SBATCH -p gpu
#SBATCH --mem-per-cpu 8000 # MB
#SBATCH --mail-type=END,FAIL
#SBATCH --output=slurm_%j.out
#SBATCH --job-name=jana_SubeventCUDAExample

set -euxo pipefail

source /etc/profile.d/modules.sh
module use /apps/modulefiles
module load cuda
module unload gcc
module load gcc/10.2.0

which gcc
which g++
export CC=`which gcc`
export CXX=`which g++`

module unload cmake
module load cmake/3.21.1
which cmake

pwd
export WKDIR=${GITHUB_WORKSPACE}
cd ${WKDIR}

# config build
mkdir -p $WKDIR/build
mkdir -p $WKDIR/install
export JANA_HOME=$WKDIR/install

srun cmake -DUSE_CUDA=On -S . -B build

# build
srun cmake --build build -j 32 --target install

# run
echo "\n Launching SubeventCUDAExample for 1s..."
srun ./install/bin/SubeventCUDAExample & \
  sleep 1  # exit effect
