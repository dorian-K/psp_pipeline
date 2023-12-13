# Use an appropriate base image
FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Berlin

# Set up environment and install dependencies
RUN apt-get update -yqq \
    && apt-get install -yqq --no-install-recommends ca-certificates libc-devtools expect avr-libc libegl1-mesa-dev libfreetype-dev libfreetype6 libfreetype6-dev binutils-avr make git cmake build-essential libelf-dev libxml2-dev libglew-dev libglfw3 libglfw3-dev pkg-config freeglut3-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /utils

COPY *.patch /utils/

RUN git clone --depth 1 https://github.com/buserror/simavr.git \
	&& cd simavr \
	&& git apply /utils/make_fix.patch \
	&& make AVR_ROOT=/usr/avr RELEASE=1 \
      ADDITIONAL_CFLAGS="-Wall -Wextra -fPIC -O2 -std=gnu99 -Wno-sign-compare -Wno-unused-parameter" \
      build-simavr \
	&& make install RELEASE=1 \
    && rm -r *

RUN git clone --depth 1 https://git.rwth-aachen.de/doriank/avrsimv2.git \
    && cd avrsimv2 \
    && cmake . -DCMAKE_BUILD_TYPE=Release \
    && make -s \
    && make install \
    && make clean 

# Set the working directory
WORKDIR /app