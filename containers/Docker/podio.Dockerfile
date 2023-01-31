
# Docker image for testing JANA/PODIO integration

FROM rootproject/root:latest
#root:6.26.10-ubuntu22.04

USER root
RUN mkdir /app
WORKDIR /app

RUN apt update -y \
 && apt install -y build-essential gdb valgrind cmake wget unzip vim libasan6 less exa bat git zlib1g-dev pip

ENV JANA_HOME /app/JANA2/install

RUN git clone -b nbrei_podio https://github.com/JeffersonLab/JANA2 /app/JANA2
RUN git clone -b v00-16-02 https://github.com/AIDASoft/podio /app/podio

RUN cd /app/JANA2 \
    && mkdir build install \
    && cmake -S . -B build \
    && cmake --build build -j 10 --target install

RUN apt install -y libyaml-dev nlohmann-json3-dev \
    && pip install jinja2 pyyaml \
    && cd /app/podio \
    && mkdir build install \
    && cd build \
    && cmake -DCMAKE_INSTALL_PREFIX=../install -DUSE_EXTERNAL_CATCH2=OFF .. \
    && make -j4 install



CMD bash

