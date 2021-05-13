cmake_minimum_required(VERSION 3.12)
include(FetchDeps.cmake)

project(dry-collection)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_find)
fetch_all_deps()

add_subdirectory(tools/dab ${PROJECT_BINARY_DIR}/tools/dab)
add_subdirectory(src ${PROJECT_BINARY_DIR}/dry1)
add_subdirectory(tests/sandbox ${PROJECT_BINARY_DIR}/tests/sandbox)