FROM ubuntu:20.04

WORKDIR /build

ENV TZ=Europe/Belgrade
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt update && apt install -y git gcc g++ cmake automake autotools-dev autoconf m4 zlib1g-dev \
 cmake libtool libssl-dev devscripts debhelper libpthread-stubs0-dev

COPY make_deb.sh /build
COPY *.zip /build
