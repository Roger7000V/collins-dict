#!/usr/bin/env sh

apk update
apk add --no-cache curl zip cmake git build-base linux-headers perl pkgconfig

BUILD_DIR=build

# Set up Ninja
cd /root/
curl -L $(curl -s https://api.github.com/repos/ninja-build/ninja/releases/latest | \
    grep zipball_url | cut -d '"' -f 4 ) -o ninja.zip
unzip ninja.zip
cd ninja-*/
cmake -B "$BUILD_DIR"
cmake --build "$BUILD_DIR"
mv "$BUILD_DIR/ninja" /usr/local/bin/

# Set up vcpkg
cd /usr/local/share/
git clone --depth=1 https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh -disableMetrics

# Build
cd /root/project/
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/usr/local/share/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_OVERLAY_TRIPLETS=triplets \
    -DVCPKG_TARGET_TRIPLET=x86-linux-static
cmake --build "$BUILD_DIR"
