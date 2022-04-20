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
#  docker build -f Dockerfile -t epsci/cuda:11.4.2-devel-ubuntu20.04 .
#
#--------------------------------------------------------------------------
#
# To run and interactive session using bash:
#
#  docker run -it --rm epsci/cuda:11.4.2-devel-ubuntu20.04
#
#--------------------------------------------------------------------------
#
# To build a singularity image from this:
#
#  singularity build epsci_cuda_11.4.2-devel-ubuntu20.04.sif docker-daemon://epsci/cuda:11.4.2-devel-ubuntu20.04
#
#--------------------------------------------------------------------------


FROM nvidia/cuda:11.4.2-devel-ubuntu20.04
LABEL maintainer "David Lawrence <davidl@jlab.org>"

# Some operations require the timezone be set and will try and prompt
# for it if it is not.
ENV TZ=US/Eastern
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Update package list from repositories
# n.b. the ";exit 0" below is to ignore an error due to the nvidia
# certificate not being verified. The right way to do this is to 
# remove the nvidia repository, but this is quicker/easier.
RUN apt-get update ; exit 0

# Install g++ and git so we can grab and compile source
RUN apt-get install -y \
	wget \
	git \
    cmake \
    make \
    python3-dev \
    python3-zmq \
    libczmq-dev \
    libxerces-c-dev \
    libx11-dev \
    libxpm-dev \
    libxft-dev \
    libxext-dev \
    libz3-dev \
    tcsh


# Copy the Dockerfile into container
ADD Dockerfile* /container/Docker

CMD bash

