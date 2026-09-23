# The Linux build image. Ubuntu 22.04 on purpose: the glibc the kit is built
# against (2.35) is the oldest it will run on, and compute nodes are not all
# current. The package list is upstream's doc/Requirements.md for Ubuntu,
# minus gnuplot and the system MPI (the kit bundles its own Open MPI).
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential gcc g++ make cmake \
        flex libfl-dev bison m4 gawk \
        zlib1g-dev libreadline-dev \
        libgmp-dev libmpfr-dev libmpc-dev \
        autoconf automake libtool pkg-config file \
        curl ca-certificates git rsync zip unzip xz-utils bzip2 python3 \
    && rm -rf /var/lib/apt/lists/*

# Unprivileged, like a node: nothing here is allowed to need root.
RUN useradd -m -u 1000 -s /bin/bash builder
USER builder
WORKDIR /repo
