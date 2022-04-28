cmake_minimum_required(VERSION 3.21)
project(collins-dict)

set(CMAKE_CXX_STANDARD 17)
set(NAME dict)

find_package(cpr REQUIRED)

find_path(LEXBOR_INCLUDE_DIR lexbor REQUIRED)
find_library(LEXBOR_LIBRARY liblexbor_static.a REQUIRED)
add_library(lexbor STATIC IMPORTED)
set_target_properties(lexbor PROPERTIES IMPORTED_LOCATION ${LEXBOR_LIBRARY})

add_executable(${NAME} ${CMAKE_SOURCE_DIR}/src/main.cpp)

target_compile_options(${NAME} PRIVATE -Wall)

target_link_libraries(${NAME} PRIVATE cpr::cpr)

target_include_directories(${NAME} PRIVATE ${LEXBOR_INCLUDE_DIR})
target_link_libraries(${NAME} PRIVATE lexbor)
