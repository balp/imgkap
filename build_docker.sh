#!/bin/bash

docker build docker -f docker/Dockerfile.18.04 -t ubuntu_build:18.04
mkdir -p build_ubuntu_18_04
docker run -v "$(pwd):/src" -w /src/build_ubuntu_18_04 --user $UID --rm ubuntu_build:18.04 cmake -G Ninja /src
docker run -v "$(pwd):/src" -w /src/build_ubuntu_18_04 --user $UID --rm ubuntu_build:18.04 cmake --build .

mkdir -p build_ubuntu_18_04_clang
docker run -v "$(pwd):/src" -w /src/build_ubuntu_18_04_clang --user $UID -e CC=clang -e CXX=clang++ --rm ubuntu_build:18.04 cmake -G Ninja /src
docker run -v "$(pwd):/src" -w /src/build_ubuntu_18_04_clang --user $UID --rm ubuntu_build:18.04 cmake --build .

docker build docker -f docker/Dockerfile.16.04 -t ubuntu_build:16.04

mkdir -p build_ubuntu_16_04
docker run -v "$(pwd):/src" -w /src/build_ubuntu_16_04 --user $UID --rm ubuntu_build:16.04 cmake -G Ninja /src
docker run -v "$(pwd):/src" -w /src/build_ubuntu_16_04 --user $UID --rm ubuntu_build:16.04 cmake --build .

mkdir -p build_ubuntu_16_04_clang
docker run -v "$(pwd):/src" -w /src/build_ubuntu_16_04_clang --user $UID --rm ubuntu_build:16.04 cmake -G Ninja /src
docker run -v "$(pwd):/src" -w /src/build_ubuntu_16_04_clang --user $UID --rm ubuntu_build:16.04 cmake --build .

#docker build docker -f docker/Dockerfile.3.6 -t alpine_build:3.6
docker build docker -f docker/Dockerfile.3.8 -t alpine_build:3.8

mkdir -p build_alpine_3.8
docker run -v "$(pwd):/src" -w /src/build_alpine_3.8 --user $UID -e CC=clang -e CXX=clang++ --rm alpine_build:3.8 cmake -G Ninja /src
docker run -v "$(pwd):/src" -w /src/build_alpine_3.8 --user $UID --rm alpine_build:3.8 cmake --build .
