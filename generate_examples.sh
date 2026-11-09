#!/bin/bash

mkdir -p build

cd ./build
cmake -G "Unix Makefiles" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DOpenGL_GL_PREFERENCE=GLVND ../examples
cd ..
