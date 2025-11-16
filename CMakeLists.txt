cmake_minimum_required (VERSION 3.0)

set(PROJECT_NAME IOFSNG)
project (${PROJECT_NAME})

find_package (PkgConfig REQUIRED)
pkg_search_module(FUSE3 REQUIRED fuse3)
set(CMAKE_INSTALL_RPATH ${FUSE3_LIBRARY_DIRS})
pkg_search_module(CURL REQUIRED libcurl)

set(CMAKE_C_FLAGS "-O0 -g3")

add_subdirectory(src)
