#!/bin/bash
set -euxo pipefail
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
pushd $SCRIPT_DIR

# build iofs-ng
#
# Note to future self: I disabled all warnings from `include` by replacing `-Iinclude` with `-isystem include` and
# thus pretending its a system include directory, since `httplib` did some C-style casting fuckery
g++ -g3 \
  -Wall -Wextra -Wpedantic -Wold-style-cast -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor \
  -std=c++23 \
  -isystem include \
  src/*.cc \
  `pkg-config fuse3 --cflags --libs` -lcurl \
  -o iofs-ng

# build the plugins
pushd plugins
PLUGINS=("sample" "lastn" "stats")
for p in "${PLUGINS[@]}"; do
  g++ -g3 -fPIC -shared -std=c++23 "$p.cc" -o "$p.so"
done
popd

popd
