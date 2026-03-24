#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
echo "Test Case 1: High throughput"
time ${SCRIPT_DIR}/high_throughput.sh
echo "Test Case 2: High iops"
time ${SCRIPT_DIR}/high_iops.sh
echo "Test Case 3: metadata load"
time ${SCRIPT_DIR}/metadata_stress.sh
