#!/bin/bash

unamestr=`uname`

if [ "$unamestr" = 'Linux' ]; then
    ./premake5 gmake
elif [ "$unamestr" = 'Darwin' ]; then
    ./premake5_macos gmake macos
fi
