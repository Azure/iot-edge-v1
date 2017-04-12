FROM ubuntu:16.04

RUN apt-get update && apt-get install -y apt-transport-https

RUN sh -c 'echo "deb [arch=amd64] https://apt-mo.trafficmanager.net/repos/dotnet-release/ xenial main" > /etc/apt/sources.list.d/dotnetdev.list' \
    && apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 417A0893 \
    && apt-get update

RUN apt-get install -y \
    curl \
    build-essential \
    openjdk-8-jdk \
    git \
    cmake \
    maven \
    valgrind \
    pkg-config \
    uuid-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    libglib2.0-dev \
    dotnet-dev-1.0.0-preview2.1-003177

WORKDIR /

RUN git clone https://github.com/Azure/azure-iot-gateway-sdk.git

WORKDIR azure-iot-gateway-sdk/

# Build node.dll for gateway's Node.js binding
RUN cp tools/build_nodejs.sh . \
    && ./build_nodejs.sh \
    && rm ./build_nodejs.sh

ENV JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64 \
    NODE_INCLUDE=/build_nodejs/dist/inc \
    NODE_LIB=/build_nodejs/dist/lib

# Trigger the population of the local package cache
ENV NUGET_XMLDOC_MODE skip
RUN mkdir /warmup \
    && cd /warmup \
    && dotnet new \
    && cd .. \
    && rm -rf warmup \
    && rm -rf /tmp/NuGetScratch
