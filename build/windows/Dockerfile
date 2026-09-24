# The Windows cross-build image. Ubuntu 24.04, not 22.04 like the Linux build:
# the kit runs on Windows, so the host's glibc is irrelevant, and 24.04 carries
# MinGW-w64 GCC 13 with posix threads (22.04 has GCC 10). Upstream's own
# Windows binaries are cross-compiled the same way, on openSUSE.
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential gcc g++ make cmake \
        g++-mingw-w64-x86-64-posix gcc-mingw-w64-x86-64-posix mingw-w64-tools \
        libz-mingw-w64-dev \
        msitools \
        flex libfl-dev bison m4 gawk \
        autoconf automake libtool pkg-config file \
        curl ca-certificates git rsync zip unzip xz-utils bzip2 python3 \
    && rm -rf /var/lib/apt/lists/* \
    && update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix \
    && update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix \
    && x86_64-w64-mingw32-g++ -v 2>&1 | grep -q 'Thread model: posix'

# Unprivileged, like a node: nothing here is allowed to need root.
# (24.04 ships a user "ubuntu" at uid 1000; the build volume is chowned to 1000.)
USER ubuntu
WORKDIR /repo
