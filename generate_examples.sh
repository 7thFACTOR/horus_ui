#!/bin/bash

mkdir build

cd ./build
cmake -G "Unix Makefiles" -DOpenGL_GL_PREFERENCE=GLVND ../examples