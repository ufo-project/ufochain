#!/bin/bash

cd /root/ufochain

# fix filename character case not work on linux
# cp ui/icons/ufo_mainnet.png ui/icons/UFO_mainnet.png
# cp ui/icons/ufo_mainnet.icns ui/icons/UFO_mainnet.icns

PATH=/opt/qt511/bin:$PATH CC=gcc-8 CXX=g++-8 cmake -DCMAKE_BUILD_TYPE=Release && make -j4
