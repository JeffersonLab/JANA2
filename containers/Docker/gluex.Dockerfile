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
RUN dnf -y install https://repo.opensciencegrid.org/osg/3.6/osg-3.6-el9-release-latest.rpm
RUN grep 'enabled=1' /etc/yum.repos.d/osg.repo

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
RUN dnf -y install libcurl-devel uClibc-devel libuuid-devel

# install the cern root suite
RUN dnf -y install root root-cling root-fftw root-foam root-fonts root-fumili \
 root-gdml root-genetic root-genvector root-geom root-geom-builder \
 root-geom-painter root-geom-webviewer root-graf root-graf-asimage \
 root-graf-fitsio root-graf-gpad root-graf-gpadv7 root-graf-gviz \
 root-graf-postscript root-graf-primitives root-graf-x11 root-graf3d \
 root-graf3d-csg root-graf3d-eve root-graf3d-eve7 root-graf3d-gl \
 root-graf3d-gviz3d root-graf3d-x3d \
 root-hbook root-hist root-hist-draw root-hist-factory root-hist-painter \
 root-histv7 root-html root-icons root-io root-io-dcache \
 root-io-sql root-io-xml root-io-xmlparser root-mathcore root-mathmore \
 root-matrix root-minuit root-minuit2 root-mlp root-montecarlo-eg \
 root-montecarlo-pythia8 root-multiproc root-net root-net-auth \
 root-net-davix root-net-http root-net-httpsniff root-net-rpdutils \
 root-netx root-notebook root-physics root-proof root-proof-bench \
 root-proof-player root-proof-sessionviewer root-quadp root-r root-r-tools \
 root-roofit root-roofit-batchcompute root-roofit-core \
 root-roofit-dataframe-helpers root-roofit-hs3 root-roofit-jsoninterface \
 root-roofit-more root-roostats root-smatrix \
 root-spectrum root-spectrum-painter root-splot root-sql-mysql \
 root-sql-odbc root-sql-pgsql root-sql-sqlite root-testsupport \
 root-tmva root-tmva-gui root-tmva-python root-tmva-r root-tmva-sofie \
 root-tmva-sofie-parser root-tmva-utils root-tpython root-tree \
 root-tree-dataframe root-tree-ntuple root-tree-ntuple-utils \
 root-tree-player root-tree-viewer root-tree-webviewer root-unfold \
 root-unuran root-vecops root-xroofit
# root packages removed from the list for unsatisfied dependencies
# root-gui-qt5webdisplay root-gui-qt6webdisplay
# root-gui-browserv7 root-gui-builder root-gui-canvaspainter root-gui-fitpanel
# root-gui-fitpanelv7 root-gui-ged root-gui-html root-gui-webdisplay
# root-gui-recorder root-gui-webgui6 root-gui root-gui-browsable
# root-roofit-common root-io-gfal
RUN dnf -y install HepMC3-rootIO python3-HepMC3-rootIO python3-jupyroot python3-root

# install the osg worker node client packages
RUN dnf -y install osg-ca-certs
RUN dnf -y install osg-wn-client
RUN dnf -y install python3-h5py python3-scipy python3-tqdm

# install some dcache client tools
RUN dnf -y install https://zeus.phys.uconn.edu/halld/gridwork/dcache-srmclient-3.0.11-1.noarch.rpm

# install some python modules
RUN pip3 install pandas
RUN pip3 install h5hep
RUN pip3 install keras
RUN pip3 install tensorflow tensorflow-decision-forests
RUN pip3 install uproot awkward
RUN pip3 install jupyterhub
RUN pip3 install jupyterlab notebook


# Fix for Silverblue's toolbox utility
RUN dnf -y install passwd sudo
RUN ln -s /run/host/opt/cvmfs /cvmfs
RUN ln -s /cvmfs/oasis.opensciencegrid.org/gluex/group /group
RUN touch /.dockerenv


