#
# Dockerfile - docker build script for a standard GlueX sim-recon 
#              container image based on alma linux 9.
#
# author: richard.t.jones at uconn.edu
# version: october 17 2023
#
# usage: [as root] $ docker build Dockerfile .
#

FROM almalinux:9


# Add JLab CA certificate
ADD http://pki.jlab.org/JLabCA.crt /etc/pki/ca-trust/source/anchors/JLabCA.crt
# Update CA certificates
RUN chmod 644 /etc/pki/ca-trust/source/anchors/JLabCA.crt && update-ca-trust


# install the necessary yum repositories
RUN dnf -y update
RUN dnf -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-9.noarch.rpm
RUN grep 'enabled=1' /etc/yum.repos.d/epel.repo
RUN /usr/bin/crb enable
# RUN dnf -y install https://repo.opensciencegrid.org/osg/3.6/osg-3.6-el9-release-latest.rpm
# RUN grep 'enabled=1' /etc/yum.repos.d/osg.repo

# install a few utility rpms
RUN dnf -y install dnf dnf-plugins-core
RUN dnf -y install python-unversioned-command
RUN dnf -y install subversion cmake make imake python3-scons patch git
RUN dnf -y install libtool which bc nano nmap-ncat xterm emacs gdb wget
RUN dnf -y install gcc-c++ gcc-gfortran boost-devel gdb-gdbserver
RUN dnf -y install bind-utils blas blas-devel dump file tcsh expat-devel
RUN dnf -y install libXt-devel openmotif-devel libXpm-devel bzip2-devel
RUN dnf -y install perl-XML-Simple perl-XML-Writer perl-File-Slurp
RUN dnf -y install mesa-libGLU-devel gsl-devel python3-future python3-devel
RUN dnf -y install xrootd-client-libs xrootd-client libXi-devel neon
RUN dnf -y install mariadb mariadb-devel python3-mysqlclient python3-psycopg2
RUN dnf -y install fmt-devel libtirpc-devel atlas rsync vim
RUN dnf -y install gfal2-all gfal2-devel gfal2-plugin-dcap gfal2-plugin-gridftp gfal2-plugin-srm
RUN dnf -y install hdf5 hdf5-devel pakchois perl-Test-Harness sqlite sqlite-devel
RUN dnf -y install java-1.8.0-openjdk java-1.8.0-openjdk-devel java-11-openjdk-devel
RUN dnf -y install java-17-openjdk-devel java-latest-openjdk-devel java-hdf5 java-runtime-decompiler
RUN dnf -y install lapack lapack-devel openmpi openmpi-devel xalan-j2
RUN dnf -y install openssh-server postgresql-server-devel postgresql-upgrade-devel
RUN dnf -y install procps-ng strace ucx valgrind xerces-c xerces-c-devel xerces-c-doc
RUN dnf -y install qt5 qt5-qtx11extras qt5-devel openblas-devel libnsl2-devel


# install the osg worker node client packages
# RUN dnf -y install osg-ca-certs
# RUN dnf -y install osg-wn-client
RUN dnf -y install python3-h5py python3-scipy python3-tqdm

# install some dcache client tools
RUN dnf -y install https://zeus.phys.uconn.edu/halld/gridwork/dcache-srmclient-3.0.11-1.noarch.rpm

#Installing cvmfs
RUN dnf -y install https://ecsft.cern.ch/dist/cvmfs/cvmfs-release/cvmfs-release-latest.noarch.rpm \
    && dnf -y install cvmfs cvmfs-config-default \
    && echo "CVMFS_HTTP_PROXY=DIRECT" | tee -a /etc/cvmfs/default.local \
    && echo "CVMFS_REPOSITORIES=oasis.opensciencegrid.org,singularity.opensciencegrid.org" | tee -a /etc/cvmfs/default.local \
    && echo "CVMFS_CLIENT_PROFILE=single" | tee -a /etc/cvmfs/default.local \
    && dnf clean all

# Fix for Silverblue's toolbox utility
RUN dnf -y install passwd sudo
RUN touch /.dockerenv


