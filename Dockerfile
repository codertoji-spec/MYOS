FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential \
    bison \
    flex \
    libgmp3-dev \
    libmpc-dev \
    libmpfr-dev \
    texinfo \
    mtools \
    xorriso \
    grub-pc-bin \
    grub-efi-amd64-bin \
    qemu-system-x86 \
    lld \
    clang \
    wget \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /os
CMD ["make"]
