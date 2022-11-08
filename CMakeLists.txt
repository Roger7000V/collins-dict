cmake_minimum_required(VERSION 3.18)

project(collins-dict)
set(CMAKE_CXX_STANDARD 17)
set(NAME dict)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

add_executable(${NAME} src/main.cpp)

if(WIN32 AND NOT MSVC)
    target_link_options(${NAME} PRIVATE -static)
endif()

find_package(cpr CONFIG REQUIRED)
target_link_libraries(${NAME} PRIVATE cpr::cpr)

find_package(lexbor CONFIG REQUIRED)
target_link_libraries(${NAME} PRIVATE lexbor)
