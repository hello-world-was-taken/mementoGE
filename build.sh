#! /bin/sh
set -e

# Check if the out directory exists, if not create it
if [ ! -d ./out ]; then
  mkdir -p out;
fi

# generate make
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DGLFW_BUILD_DOCS=OFF \
      -S . -B ./out

# run the generated make
cd out
make -j8