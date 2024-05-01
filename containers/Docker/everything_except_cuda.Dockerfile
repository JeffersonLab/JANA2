
# Docker image for testing JANA with all options enabled except CUDA

FROM rootproject/root:latest
# This is an Ubuntu >= 22.04

USER root
RUN mkdir /app
WORKDIR /app

RUN apt update -y \
 && apt install -y build-essential gdb valgrind cmake wget unzip vim libasan6 less exa bat git zlib1g-dev pip \
                   python3-dev python3-zmq libczmq-dev libxerces-c-dev libx11-dev libxpm-dev libxft-dev libxext-dev \
                   libyaml-dev python3-jinja2 python3-yaml libz3-dev


RUN git clone -b v00-99 https://github.com/AIDASoft/podio /app/podio

RUN cd /app/podio \
    && mkdir build install \
    && cd build \
    && cmake -DCMAKE_INSTALL_PREFIX=../install -DUSE_EXTERNAL_CATCH2=OFF .. \
    && make -j4 install

ENV JANA_HOME /app/JANA2/install

# RUN cd /app/JANA2 \
#    && mkdir build install \
#    && cmake -S . -B build \
#    && cmake --build build -j 10 --target install

CMD bash

