FROM ubuntu:16.04

ENV DEBIAN_FRONTEND noninteractive

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
    && rm -rf /var/lib/apt/lists/* \
    && cd /usr/bin \
    && ln -s python2.7 python

ENV WRLINUX_INSTALLER wrlinux-7.0.0.13-glibc-x86_64-intel_baytrail_64-wrlinux-image-idp-sdk.sh
COPY $WRLINUX_INSTALLER /
RUN /$WRLINUX_INSTALLER \
    && rm /$WRLINUX_INSTALLER
