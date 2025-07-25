FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

ENV DEBIAN_FRONTEND=noninteractive

# build
RUN apt-get update && apt-get install -y \
    build-essential cmake meson ninja-build

# for documentation
RUN apt-get install -y openjdk-17-jre ruby-full && \
    gem install asciidoctor asciidoctor-diagram asciidoctor-diagram-plantuml --no-document

# test
RUN git clone https://github.com/google/googletest.git -b v1.17.0 && \
    cd googletest && \
    mkdir build && \
    cd build && \
    cmake .. -DBUILD_SHARED_LIBS=ON && \
    make && \
    make install && \
    cd ../.. && \
    rm -rf googletest

# senscord
RUN apt-get install -y libxml2-dev debhelper devscripts libcamera-dev \
    libjpeg-dev libdrm-dev libexif-dev libtiff5-dev libpng-dev libboost-all-dev python3-kconfiglib nlohmann-json3-dev \
    python3-build python3-stdeb dh-python python3-msgpack
