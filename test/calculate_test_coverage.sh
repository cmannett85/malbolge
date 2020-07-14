#!/usr/bin/env bash

# Copyright Cam Mannett 2020
#
# See LICENSE file
#

# Run from inside the test folder.
# - First argument is the top-level build directory and is required
# - Second argument is optional, and defined the gcov tool (defaults to gcov-10)
if [ -z "$1" ]; then
    echo "Must pass build directory"
    exit 1
fi

BUILD_DIR=$1
GCOV=${2:-gcov-10}

SRC_PATH=${PWD}
OLD_COVERAGE="$(cat ./old_coverage)"

cd ${BUILD_DIR}/test/CMakeFiles/malbolge_test_coverage.dir
lcov --directory . --capture --output-file temp.info --rc geninfo_gcov_tool=${GCOV}
lcov --remove temp.info "/usr/include/*" --output-file malbolge.info

NEW_COVERAGE="$(lcov --summary malbolge.info | awk 'NR==3 {print $2+0}')"
DIFF=$(echo "$NEW_COVERAGE - $OLD_COVERAGE" | bc);
echo "New coverage: ${NEW_COVERAGE}%, previous coverage: ${OLD_COVERAGE}%, diff: ${DIFF}"
if (( $(echo "${DIFF} < -1.0" | bc -l) )); then
    exit 1
fi

# If the coverage has improved, then write the new value back to the cache
if (( $(echo "${DIFF} > 0.0" | bc -l) )); then
    echo ${DIFF} > ${SRC_PATH}/old_coverage
fi
