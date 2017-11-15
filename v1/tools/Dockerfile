FROM debian:jessie-slim

#### Update package sources
    ## For Java
RUN echo "deb http://ppa.launchpad.net/webupd8team/java/ubuntu trusty main" | tee /etc/apt/sources.list.d/webupd8team-java.list \
    && echo "deb-src http://ppa.launchpad.net/webupd8team/java/ubuntu trusty main" | tee -a /etc/apt/sources.list.d/webupd8team-java.list \
    && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys EEA14886
    ## For CMake
RUN echo "deb http://ftp.debian.org/debian jessie-backports main" | tee /etc/apt/sources.list.d/cmake.list
RUN apt-get update

#### Install Java
RUN mkdir -p /usr/share/man/man1/ \
    && echo debconf shared/accepted-oracle-license-v1-1 select true | debconf-set-selections \
    && echo debconf shared/accepted-oracle-license-v1-1 seen true | debconf-set-selections \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y oracle-java8-installer oracle-java8-set-default
ENV JAVA_HOME /usr/lib/jvm/java-8-oracle

#### Install .NET Core dependencies
RUN apt-get install -y --no-install-recommends \
        software-properties-common \
        libc6 \
        libcurl3 \
        libgcc1 \
        libgssapi-krb5-2 \
        libicu52 \
        liblttng-ust0 \
        libssl1.0.0 \
        libstdc++6 \
        libunwind8 \
        libuuid1 \
        zlib1g

#### Install Libuv dependencies
RUN apt-get install -y automake libtool \
    && DEBIAN_FRONTEND=noninteractive apt-get -t jessie-backports install -y cmake

#### Install IoT Edge dependencies
RUN apt-get install -y \
        libglib2.0-dev \
        ca-certificates \
        maven \
        python2.7 \
        build-essential \
        curl \
        libcurl4-openssl-dev \
        git \
        cmake \
        libssl-dev \
        uuid-dev \
        valgrind \
        wget

#### Cleanup
RUN rm -rf /var/cache/oracle-jdk8-installer \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

#### Install .NET Core SDK
ENV DOTNET_SDK_VERSION 1.0.1
ENV DOTNET_SDK_DOWNLOAD_URL https://dotnetcli.blob.core.windows.net/dotnet/Sdk/$DOTNET_SDK_VERSION/dotnet-dev-debian-x64.$DOTNET_SDK_VERSION.tar.gz

RUN curl -SL $DOTNET_SDK_DOWNLOAD_URL --output dotnet.tar.gz \
    && mkdir -p /usr/share/dotnet \
    && tar -zxf dotnet.tar.gz -C /usr/share/dotnet \
    && rm dotnet.tar.gz \
    && ln -s /usr/share/dotnet/dotnet /usr/bin/dotnet

# Trigger the population of the local package cache
ENV NUGET_XMLDOC_MODE skip
RUN mkdir warmup \
    && cd warmup \
    && dotnet new \
    && cd .. \
    && rm -rf warmup \
    && rm -rf /tmp/NuGetScratch

#### Install Node.js
RUN wget https://raw.githubusercontent.com/Azure/iot-edge/master/tools/build_nodejs.sh -P /tools/ \
    && chmod +x /tools/build_nodejs.sh \
    && rm -fr /usr/bin/python && ln -s /usr/bin/python2.7 /usr/bin/python \
    && /tools/build_nodejs.sh

ENV NODE_INCLUDE /build_nodejs/dist/inc
ENV NODE_LIB /build_nodejs/dist/lib

#### Clone/build latest IoT Edge bits
ENTRYPOINT  git clone https://github.com/Azure/iot-edge /iotedge \
            && /iotedge/tools/build.sh \
                --run-unittests \
                --enable-java-binding \
                --enable-dotnet-core-binding \
                --enable-nodejs-binding \
            && bash
