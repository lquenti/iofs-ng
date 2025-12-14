#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

find "${SCRIPT_DIR}/../src/" -iname '*.hh' -o -iname '*.cc' | xargs clang-format -i
