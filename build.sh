#! /bin/sh

# Check if the out directory exists, if not create it
if [ ! -d ./out ]; then
  mkdir -p out;
fi

# This script is used to run cmake and make for the project
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DGLFW_BUILD_DOCS=OFF \
      -S . -B ./out

# Run make to build the project
cd out
make