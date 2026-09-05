FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y \
        build-essential \
        cmake \
        libssl-dev \
        iproute2 \
        iputils-ping \
        tcpdump \
        && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /workspace