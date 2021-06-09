cmake_minimum_required(VERSION 3.12)
include(FetchDeps.cmake)

project(dry-collection)

option(DRY_BUILD_DAB "Build dab" ON)
option(DRY_BUILD_DRY1 "Build dry1" ON)
option(DRY_BUILD_TESTS "Build tests" ON)

fetch_all_deps()

if (DRY_BUILD_DAB)
    add_subdirectory(tools/dab ${PROJECT_BINARY_DIR}/tools/dab)
endif()

if (DRY_BUILD_DRY1)
    add_subdirectory(src ${PROJECT_BINARY_DIR}/dry1)
endif()

if (DRY_BUILD_TESTS)
    add_subdirectory(tests/sandbox ${PROJECT_BINARY_DIR}/tests/sandbox)
endif()