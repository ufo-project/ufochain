#!/bin/bash

apt-get -y update
add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt-get -y update
apt-get -y install wget gcc-8 unzip libssl1.0.0 software-properties-common
apt-get -y install --only-upgrade libstdc++6

add-apt-repository -y ppa:beineri/opt-qt-5.11.0-bionic
add-apt-repository -y ppa:mhier/libboost-latest
apt-get -y update
apt-get -y upgrade
apt-get -y install g++-8 libssl-dev curl wget git make
apt-get -y install libgl1-mesa-dev zlib1g-dev libboost1.67-dev
apt-get -y install qt511base qt511declarative qt511svg qt511tools
apt-get clean

wget https://cmake.org/files/v3.13/cmake-3.13.0-Linux-x86_64.sh
sh ./cmake-3.13.0-Linux-x86_64.sh --skip-license --prefix=/usr

cd /root && git clone https://github.com/ufo-project/ufochain.git
