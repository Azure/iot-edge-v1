FROM ubuntu:16.04

ENV DEBIAN_FRONTEND noninteractive
ENV TOOLCHAIN_URL http://downloadmirror.intel.com/25028/eng/edison-sdk-linux64-ww25.5-15.zip

RUN apt-get update
RUN apt-get install -y apt-utils
RUN apt-get install -y --no-install-recommends \
    bzip2 \
    ca-certificates \
    cmake \
    curl \
    file \
    g++ \
    git \   
    make \
    pkg-config \
    python2.7 \
    unzip \
    && rm -rf /var/lib/apt/lists/* \
    && cd /usr/bin \
    && ln -s python2.7 python

RUN curl -sSL $TOOLCHAIN_URL --output toolchain.zip \
    && unzip toolchain.zip \
    && rm toolchain.zip
RUN INSTALLER=$(ls | grep .sh) \
    && ./$INSTALLER \
    && rm $INSTALLER
