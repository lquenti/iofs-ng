#/bin/bash
set -euxo pipefail
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
pushd $SCRIPT_DIR

# build iofs-ng
g++ -g3 \
  -Wall -Wextra -Wpedantic -Wold-style-cast -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor \
  -std=c++23 \
  -Iinclude \
  src/*.cc \
  `pkg-config fuse3 --cflags --libs` -lcurl \
  -o iofs-ng

# build the plugins
pushd plugins
g++ -g3 -fPIC -shared -std=c++23 dummy.cc -o dummy.so
popd

popd
