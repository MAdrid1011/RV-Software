FROM debian:12.12-slim@sha256:d5d3f9c23164ea16f31852f95bd5959aad1c5e854332fe00f7b3a20fcc9f635c

RUN apt-get -o Acquire::Retries=5 update \
    && apt-get -o Acquire::Retries=5 install -y --no-install-recommends \
        bc \
        bison \
        build-essential \
        ca-certificates \
        cpio \
        curl \
        device-tree-compiler \
        file \
        flex \
        git \
        libelf-dev \
        libncurses-dev \
        libssl-dev \
        patch \
        perl \
        python3 \
        qemu-system-misc \
        rsync \
        unzip \
        wget \
        xz-utils \
    && rm -rf /var/lib/apt/lists/*

ENV LANG=C.UTF-8
