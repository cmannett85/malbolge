#!/usr/bin/env bash

# Copyright Cam Mannett 2020
#
# See LICENSE file
#

# Only for execution inside a Github Action
# - First argument is GITHUB_WORKSPACE
# - Second argument is the base directory to operate in
# - Third argument is the documentation staging area
if [ -z "$1" ]; then
    echo "Must pass GITHUB_WORKSPACE directory"
    exit 1
fi
if [ -z "$2" ]; then
    echo "Must pass base directory"
    exit 1
fi
if [ -z "$3" ]; then
    echo "Must pass staging area directory"
    exit 1
fi

GITHUB_WORKSPACE=$1
BASE_DIR=$2
STAGING_DIR=$3

build_from_cmake() {
    local TAG=$1

    cd ${BASE_DIR}/docs
    cmake -DDOCS_ONLY=ON ../${TAG}
    make -j4 documentation
    rm -rf ${BASE_DIR}/docs
}

build_manually() {
    local TAG=$1

    cd ${BASE_DIR}/${TAG}/docs
    doxygen ./Doxyfile
}

cd ${GITHUB_WORKSPACE}
TAGS=$(git tag)

for TAG in $TAGS; do
    # Check out the tag
    mkdir -p ${BASE_DIR}/docs
    cd ${BASE_DIR}
    git clone https://github.com/cmannett85/malbolge.git -b $TAG --depth 1 $TAG

    # If there's no documentation CMake script (earlier versions) then just
    # manually run Doxygen
    if [ -e ${BASE_DIR}/${TAG}/cmake/build_types/documentation.cmake ]; then
        build_from_cmake $TAG
    else
        build_manually $TAG
    fi

    mkdir -p ${STAGING_DIR}/${TAG}
    cp -rf ${BASE_DIR}/${TAG}/docs/doxygen/html/* ${STAGING_DIR}/${TAG}
    rm -rf ${BASE_DIR}/${TAG}

    # Update the landing page link
    sed -i "s/<\/ul>/    <li><a href=\"https:\/\/cmannett85.github.io\/malbolge\/${TAG}\">${TAG}<\/a><\/li>\n&/" ${GITHUB_WORKSPACE}/docs/landing_page/index.html
done
