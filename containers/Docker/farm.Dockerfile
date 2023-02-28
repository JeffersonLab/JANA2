# JANA docker image
#
# This will create a Docker image containing JANA source
# and binaries.
#
# This Dockerfile is written by:
#
# David Lawrence
#
# Instructions:
#
#--------------------------------------------------------------------------
#
# To build and push the container:
#
# docker build -f Dockerfile -t jana .
#
# docker tag jana jeffersonlab/jana
#
# docker push jeffersonlab/jana
#
#--------------------------------------------------------------------------
#
# To run and interactive session using bash:
#
#  docker run -it --rm jeffersonlab/jana bash
#  # jana -PPLUGINS=JTest -PNTHREADS=4 dummy
#
#
#  To run as an app with a binary compatible 
# plugin and input file from the host:
#
#  docker run -it --rm  -v /path/to/plugins:/plugins -v /path/to/work:/work jeffersonlab/jana -PMyPlugin -PNTHREADS=4 /work/file
#--------------------------------------------------------------------------

FROM centos:7.7.1908
LABEL maintainer "David Lawrence <davidl@jlab.org>"

# Setup environment variables
ENV JANA_HOME /opt/JANA2/Linux_CentOS7-x86_64-gcc4.8.5
#ENV JANA_HOME /opt/JANA2
ENV JANA_PLUGIN_PATH /usr/lib64/jana/plugins:/plugins
ENV LD_LIBRARY_PATH ${JANA_PLUGIN_PATH}:/usr/local/lib64

# Add enterprise linux repositories
RUN yum install -y epel-release

# Install g++ and git so we can grab and compile source
RUN yum install -y \
	gcc-c++ \
	git \
    cmake3 \
    make \
    python3-tkinter \
    python3-devel \
    python36-zmq \
    tcsh \
	&& yum clean all

# Download JANA source
RUN git clone https://github.com/JeffersonLab/JANA2 /opt/JANA2

# Compile and install JANA source
RUN cd /opt/JANA2 \
    && git checkout davidl_janacontrol \
    && mkdir build \
    && cd build \
    && cmake3 .. \
    && make -j8 install \
	&& mkdir /usr/lib64/jana \
	&& ln -s ${JANA_HOME}/Linux_*/plugins /usr/lib64/jana \
	&& for f in `ls /opt/JANA2/Linux_CentOS7-x86_64-gcc4.8.5/bin/*` ; do ln -s $f /usr/bin ; done\
    && cd .. \
	&& rm -rf build/*

# Copy the Dockerfile into container
ADD Dockerfile* /container/Docker/jana/

CMD bash

