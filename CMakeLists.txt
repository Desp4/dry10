cmake_minimum_required(VERSION 3.12)
include(FetchDeps.cmake)

project(dry-collection)

option(DRY_BUILD_DAB "Build dab" ON)
option(DRY_BUILD_DRY1 "Build dry1" ON)
option(DRY_BUILD_TESTS "Build tests" ON)

if (DRY_BUILD_TESTS)
    set(DRY_BUILD_DAB ON)
    set(DRY_BUILD_DRY1 ON)
endif()

fetch_all_deps()

add_library(dry_common INTERFACE)
target_include_directories(dry_common INTERFACE ${PROJECT_SOURCE_DIR}/common)

# DRY_BUILD_DAB is checked inside
add_subdirectory(tools/dab ${PROJECT_BINARY_DIR}/tools/dab)

if (DRY_BUILD_DRY1)
    add_subdirectory(src ${PROJECT_BINARY_DIR}/dry1)
endif()

if (DRY_BUILD_TESTS)
    add_subdirectory(tests ${PROJECT_BINARY_DIR}/tests)
endif()