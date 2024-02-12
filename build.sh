#! /bin/sh

# This script is used to run cmake and make for the project
cmake -DGLFW_BUILD_DOCS=OFF -S . -B ./build

# Run make to build the project
cd build
make