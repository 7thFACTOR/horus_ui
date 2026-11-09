#!/bin/bash

sh ./generate_examples.sh
cd ./build
make config=release
cd ..
