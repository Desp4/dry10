cmake_minimum_required(VERSION 3.12)

project(dry_sandbox)

set(CMAKE_CXX_STANDARD 20)

add_executable(dry_sandbox "${PROJECT_SOURCE_DIR}/src/main.cpp")

target_include_directories(dry_sandbox PRIVATE ${DRY1_MAIN_SRC})
target_link_libraries(dry_sandbox PRIVATE dry1)