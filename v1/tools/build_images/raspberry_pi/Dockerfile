# Cross-compile environment for amd64 Linux host targeting Raspberry Pi 2/3/+ and Raspbian Stretch
FROM ubuntu:16.04

ENV DEBIAN_FRONTEND noninteractive
ENV SYSROOT_DIR sysroot-glibc-linaro-2.25-2019.02-arm-linux-gnueabihf
ENV SYSROOT_URL https://releases.linaro.org/components/toolchain/binaries/latest-7/arm-linux-gnueabihf/sysroot-glibc-linaro-2.25-2019.02-arm-linux-gnueabihf.tar.xz
ENV TOOLCHAIN_URL https://releases.linaro.org/components/toolchain/binaries/latest-7/arm-linux-gnueabihf/gcc-linaro-7.4.1-2019.02-x86_64_arm-linux-gnueabihf.tar.xz

RUN apt-get update
RUN apt-get install -y apt-utils
RUN apt-get install -y --no-install-recommends \
    ca-certificates \
    cmake \
    curl \
    g++ \
    gcc \
    git \
    make \
    pkg-config-arm-linux-gnueabihf \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /toolchain
RUN curl -sSL $TOOLCHAIN_URL | tar Jx
RUN curl -sSL $SYSROOT_URL | tar Jx
WORKDIR /

ENV SYSROOT /toolchain/$SYSROOT_DIR

# /lib and /usr in your context folder come from a Raspberry Pi with Raspbian
# Stretch that has all the needed libraries to build IoT Edge v1. On your Pi,
# convert absolute symlinks to relative like this:
#   sudo apt-get update
#   sudo apt-get install symlinks
#   cd /usr/lib/arm-linux-gnueabihf/
#   sudo symlinks -c .
# Then on your cross-compile machine, transfer the folders from your Pi in a way
# that will preserve symlinks, e.g.:
#   rsync -avr pi@<ipaddr>:/{lib,usr} <dest>

# curl
COPY lib/arm-linux-gnueabihf/libcom_err* lib/arm-linux-gnueabihf/libgcrypt* lib/arm-linux-gnueabihf/libgpg-error* lib/arm-linux-gnueabihf/libidn* lib/arm-linux-gnueabihf/libkeyutils* lib/arm-linux-gnueabihf/libz* \
    $SYSROOT/lib/arm-linux-gnueabihf/
COPY usr/include/arm-linux-gnueabihf/curl \
    $SYSROOT/usr/include/arm-linux-gnueabihf/curl/
COPY usr/lib/arm-linux-gnueabihf/libcurl* usr/lib/arm-linux-gnueabihf/libffi* usr/lib/arm-linux-gnueabihf/libgmp* usr/lib/arm-linux-gnueabihf/libgnutls* usr/lib/arm-linux-gnueabihf/libgssapi_krb5* usr/lib/arm-linux-gnueabihf/libhogweed* usr/lib/arm-linux-gnueabihf/libidn2* usr/lib/arm-linux-gnueabihf/libk5crypto* usr/lib/arm-linux-gnueabihf/libkrb5* usr/lib/arm-linux-gnueabihf/liblber-2.4* usr/lib/arm-linux-gnueabihf/libldap_r-2.4* usr/lib/arm-linux-gnueabihf/libnettle* usr/lib/arm-linux-gnueabihf/libnghttp2* usr/lib/arm-linux-gnueabihf/libp11-kit* usr/lib/arm-linux-gnueabihf/libpsl* usr/lib/arm-linux-gnueabihf/librtmp* usr/lib/arm-linux-gnueabihf/libsasl2* usr/lib/arm-linux-gnueabihf/libssh2* usr/lib/arm-linux-gnueabihf/libtasn1* usr/lib/arm-linux-gnueabihf/libunistring* usr/lib/arm-linux-gnueabihf/libz* \
    $SYSROOT/usr/lib/arm-linux-gnueabihf/
COPY usr/lib/arm-linux-gnueabihf/pkgconfig/libcurl.* \
    $SYSROOT/usr/lib/arm-linux-gnueabihf/pkgconfig/

# openssl
COPY usr/include/arm-linux-gnueabihf/openssl \
    $SYSROOT/usr/include/arm-linux-gnueabihf/openssl/
COPY usr/include/openssl \
    $SYSROOT/usr/include/openssl/
COPY usr/lib/arm-linux-gnueabihf/libcrypto* usr/lib/arm-linux-gnueabihf/libssl* \
    $SYSROOT/usr/lib/arm-linux-gnueabihf/
COPY usr/lib/arm-linux-gnueabihf/pkgconfig/libcrypto* usr/lib/arm-linux-gnueabihf/pkgconfig/libssl* usr/lib/arm-linux-gnueabihf/pkgconfig/openssl* \
    $SYSROOT/usr/lib/arm-linux-gnueabihf/pkgconfig/

# uuid
COPY lib/arm-linux-gnueabihf/libuuid* \
    $SYSROOT/lib/arm-linux-gnueabihf/
COPY usr/include/uuid \
    $SYSROOT/usr/include/uuid/
COPY usr/lib/arm-linux-gnueabihf/libuuid* \
    $SYSROOT/usr/lib/arm-linux-gnueabihf/
COPY usr/lib/arm-linux-gnueabihf/pkgconfig/uuid* \
    $SYSROOT/usr/lib/arm-linux-gnueabihf/pkgconfig/

# gio
COPY lib/arm-linux-gnueabihf/libblkid* lib/arm-linux-gnueabihf/libglib-2.0* lib/arm-linux-gnueabihf/libmount* lib/arm-linux-gnueabihf/libpcre* lib/arm-linux-gnueabihf/libselinux* \
    $SYSROOT/lib/arm-linux-gnueabihf/
COPY usr/include/gio-unix-2.0 \
    $SYSROOT/usr/include/gio-unix-2.0/
COPY usr/include/glib-2.0 \
    $SYSROOT/usr/include/glib-2.0/
COPY usr/lib/arm-linux-gnueabihf/glib-2.0 \
    $SYSROOT/usr/lib/arm-linux-gnueabihf/glib-2.0/
COPY usr/lib/arm-linux-gnueabihf/libgio-2.0* usr/lib/arm-linux-gnueabihf/libglib-2.0* usr/lib/arm-linux-gnueabihf/libgobject-2.0* usr/lib/arm-linux-gnueabihf/libgmodule-2.0* \
    $SYSROOT/usr/lib/arm-linux-gnueabihf/
COPY usr/lib/arm-linux-gnueabihf/pkgconfig/gio*-2.0* usr/lib/arm-linux-gnueabihf/pkgconfig/glib-2.0* usr/lib/arm-linux-gnueabihf/pkgconfig/gobject-2.0* usr/lib/arm-linux-gnueabihf/pkgconfig/gmodule*-2.0* usr/lib/arm-linux-gnueabihf/pkgconfig/libpcre* \
    $SYSROOT/usr/lib/arm-linux-gnueabihf/pkgconfig/
