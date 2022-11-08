#!/usr/bin/env sh

set -e

get_release() {
    LINK=https://github.com/$1/$2
    curl -L $LINK/archive/refs/tags/$(git ls-remote --refs --sort=version:refname --tags \
        $LINK.git | cut -d / -f 3 | tail -n 1).tar.gz -o $2.tar.gz
    tar -xf $2.tar.gz
}

apk add build-base curl git linux-headers perl pkgconf zip

NPROC=$(nproc)
LIB_DIR=/usr/local/share

# Set up CMake
apk add cmake --repository=https://dl-cdn.alpinelinux.org/alpine/edge/main
if ! cmake; then
    # Incompatible CMake, compile manually
    apk del cmake
    apk add openssl-dev
    cd $HOME/
    get_release Kitware CMake
    cd CMake-*/
    ./bootstrap
    make -j $NPROC
    make install
fi

# Set up Ninja
cd $HOME/
get_release ninja-build ninja
cd ninja-*/
cmake -B $BUILD_DIR \
    -DCMAKE_BUILD_TYPE=Release
cmake --build $BUILD_DIR -j $NPROC --target install

# Set up vcpkg
cd $LIB_DIR/
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh -disableMetrics

# Build
cd $HOME/$PROJ_DIR/
cmake -B $BUILD_DIR \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$LIB_DIR/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x86-linux
cmake --build $BUILD_DIR -j $NPROC
