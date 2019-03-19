FROM debian:jessie-slim

ENV DEBIAN_FRONTEND noninteractive

# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=863199
RUN mkdir -p /usr/share/man/man1

# For OpenJDK
RUN echo 'deb http://deb.debian.org/debian jessie-backports main' > /etc/apt/sources.list.d/backports.list

RUN apt-get update
RUN apt-get install -y apt-utils
RUN apt-get install -y --no-install-recommends \
    apt-transport-https \
    automake \
    ca-certificates \
    curl \
    g++ \
    gcc \
    git \
    libc6 \
    libc6-dev \
    libcurl3 \
    libcurl4-openssl-dev \
    libgcc1 \
    libglib2.0-dev \
    libgssapi-krb5-2 \
    libicu52 \
    liblttng-ust0 \
    libssl-dev \
    libssl1.0.0 \
    libstdc++6 \
    libtool \
    libunwind8 \
    libuuid1 \
    make \
    pkg-config \
    python2.7 \
    uuid-dev \
    zlib1g

# Java & CMake from jessie-backports
RUN apt-get install -t jessie-backports -y --no-install-recommends \
    cmake \
    openjdk-8-jdk-headless
ENV JAVA_HOME /usr/lib/jvm/java-8-openjdk-amd64

# Maven
ENV MAVEN_VERSION 3.6.0
ENV MAVEN_DOWNLOAD_URL http://apache.mirrors.hoobly.com/maven/maven-3/$MAVEN_VERSION/binaries/apache-maven-$MAVEN_VERSION-bin.tar.gz
RUN curl -SL $MAVEN_DOWNLOAD_URL --output maven.tar.gz \
    && mkdir -p /usr/share/maven \
    && tar -zxf maven.tar.gz -C /usr/share/maven --strip=1 \
    && ln -s /usr/share/maven/bin/mvn /usr/bin/mvn

# .NET Core
ENV DOTNET_SDK_VERSION 1.1.12
ENV DOTNET_SDK_DOWNLOAD_URL https://dotnetcli.blob.core.windows.net/dotnet/Sdk/$DOTNET_SDK_VERSION/dotnet-dev-debian-x64.$DOTNET_SDK_VERSION.tar.gz
RUN curl -SL $DOTNET_SDK_DOWNLOAD_URL --output dotnet.tar.gz \
    && mkdir -p /usr/share/dotnet \
    && tar -zxf dotnet.tar.gz -C /usr/share/dotnet \
    && ln -s /usr/share/dotnet/dotnet /usr/bin/dotnet

# Trigger the population of the local package cache
ENV NUGET_XMLDOC_MODE skip
RUN mkdir warmup \
    && cd warmup \
    && dotnet new \
    && cd .. \
    && rm -rf warmup \
    && rm -rf /tmp/NuGetScratch

# Node.js
ENV NODE_VERSION node_9.x
RUN curl -SL https://deb.nodesource.com/gpgkey/nodesource.gpg.key | apt-key add - \
    && echo "deb https://deb.nodesource.com/$NODE_VERSION jessie main" > /etc/apt/sources.list.d/nodesource.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends nodejs

# Cleanup
RUN rm -rf /var/lib/apt/lists/* \
    && rm maven.tar.gz \
    && rm dotnet.tar.gz
