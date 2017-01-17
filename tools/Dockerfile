FROM ubuntu:14.04

RUN apt-get clean && \
    apt-get update && \
    apt-get install -y \
        software-properties-common && \
    apt-add-repository ppa:webupd8team/java && \
    echo oracle-java8-installer shared/accepted-oracle-license-v1-1 select true | sudo /usr/bin/debconf-set-selections && \
    apt-get update && \
    apt-get install -y \
        oracle-java8-installer \
        oracle-java8-set-default \
        maven \
        python2.7 \
        curl \
        build-essential \
        libcurl4-openssl-dev \
        git \
        cmake \
        libssl-dev \
        uuid-dev \
        valgrind \
        libglib2.0-dev

COPY build_nodejs.sh /tools/

RUN /tools/build_nodejs.sh

ENV JAVA_HOME /usr/lib/jvm/java-8-oracle

ENV NODE_INCLUDE /build_nodejs/dist/inc
ENV NODE_LIB /build_nodejs/dist/lib

ENV GATEWAY_REPO https://github.com/Azure/azure-iot-gateway-sdk
ENV COMMIT_ID master

ENTRYPOINT rm -rf /gateway && \
           git clone ${GATEWAY_REPO} /gateway && \
           git -C /gateway checkout ${COMMIT_ID} && \
           git -C /gateway submodule update --init && \
           /gateway/tools/build.sh --run-unittests -rv --enable-java-binding --enable-nodejs-binding && \
           bash
