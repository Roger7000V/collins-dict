vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO lexbor/lexbor
    REF 842754c0863c0d8d03a6ab8bb449f631139d99d5
    SHA512 a82d8a7d784d328b86bbc9b35cb897e863d764b085962e3ea4f7059452c600f862229bf86408049e894ef02cb949fb786760316d2596bd6f6794fe874c3f0520
)

string(COMPARE EQUAL ${VCPKG_LIBRARY_LINKAGE} static LEXBOR_BUILD_STATIC)
string(COMPARE EQUAL ${VCPKG_LIBRARY_LINKAGE} dynamic LEXBOR_BUILD_SHARED)
vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    OPTIONS
        -DLEXBOR_BUILD_STATIC=${LEXBOR_BUILD_STATIC}
        -DLEXBOR_BUILD_SHARED=${LEXBOR_BUILD_SHARED}
    PREFER_NINJA
)

vcpkg_install_cmake()

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/include/${PORT}/html/tree/insertion_mode)

file(
    INSTALL ${CMAKE_CURRENT_LIST_DIR}/${PORT}-config.cmake
    DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT}
)
file(
    INSTALL ${SOURCE_PATH}/LICENSE
    DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT}
    RENAME copyright
)
