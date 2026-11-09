#!/bin/bash

sh ./generate_examples.sh

cd ./build
make -j4
cd ..
