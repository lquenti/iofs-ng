#/bin/bash
set -euxo pipefail
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
pushd $SCRIPT_DIR
./build.sh
mkdir -p ./mount/{fake,real}
./iofs-ng -fd -p ./plugins/sample.so -p ./plugins/lastn.so -p ./plugins/stats.so mount/fake/ mount/real/
popd
