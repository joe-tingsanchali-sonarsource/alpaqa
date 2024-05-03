#!/usr/bin/env bash
cd "$( dirname "${BASH_SOURCE[0]}" )"/..

prefix="$1" # Installation directory prefix
build_type="${2:-RelWithDebInfo}"
version="master"
commit="c1d637433e3b3f9012b226c2c9125c494b470ae6"

if [ -z "$prefix" ]; then
    if [ -z "$VIRTUAL_ENV" ]; then
        echo "No active virtual environment, refusing to install."
        exit 1
    else
        prefix="$VIRTUAL_ENV"
    fi
fi

set -ex
export CMAKE_PREFIX_PATH="$prefix:$CMAKE_PREFIX_PATH"
export PKG_CONFIG_PATH="$prefix/lib/pkgconfig:$PKG_CONFIG_PATH"

pushd "${TMPDIR:-/tmp}"

# Eigen
[ -d eigen ] \
 || git clone --single-branch --branch "$version" \
    https://gitlab.com/libeigen/eigen.git
pushd eigen
git checkout "$commit"
cmake -S. -Bbuild \
    -G "Ninja Multi-Config" \
    -D CMAKE_INSTALL_PREFIX="$prefix" \
    -D CMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build -j --config $build_type
cmake --install build --config $build_type
popd

popd
