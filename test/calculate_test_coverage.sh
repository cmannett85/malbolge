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

OLD_COVERAGE="$(cat ./old_coverage)"

cd ${BUILD_DIR}/test/CMakeFiles/malbolge_test.dir
lcov --directory . --capture --output-file temp.info --rc geninfo_gcov_tool=${GCOV}
lcov --remove temp.info "/usr/include/*" --output-file malbolge.info

NEW_COVERAGE="$(lcov --summary malbolge.info | awk 'NR==3 {print $2+0}')"
echo "New coverage: ${NEW_COVERAGE}%, previous coverage: ${OLD_COVERAGE}%"
if (( $(echo "($NEW_COVERAGE - $OLD_COVERAGE) < -1.0" | bc -l) )); then
    exit 1
fi
