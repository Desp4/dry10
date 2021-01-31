cmake_minimum_required(VERSION 3.12)

project(dry1)

set(CMAKE_CXX_STANDARD 20)

set(DRY1_SHPB_INC "${PROJECT_SOURCE_DIR}/tools/dab/include")
set(DRY1_MAIN_SRC "${PROJECT_SOURCE_DIR}/src")

add_subdirectory("${PROJECT_SOURCE_DIR}/tools/dab" "${CMAKE_CURRENT_BINARY_DIR}/dab_build")
add_subdirectory("${PROJECT_SOURCE_DIR}/src" "${CMAKE_CURRENT_BINARY_DIR}/dry_build")

# tests
add_subdirectory("${PROJECT_SOURCE_DIR}/tests/sandbox" "${CMAKE_CURRENT_BINARY_DIR}/sandbox_build")