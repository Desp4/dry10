cmake_minimum_required(VERSION 3.12)

project(dry_sandbox)

set(CMAKE_CXX_STANDARD 20)

add_executable(dry_sandbox "${PROJECT_SOURCE_DIR}/src/main.cpp")

target_link_libraries(dry_sandbox PRIVATE dry1)

add_custom_target(run_sandbox_dab
    COMMAND dab "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_BINARY_DIR}/assets/assets.zip"
    COMMENT "archiving assets for sandbox")

if (NOT EXISTS "${PROJECT_BINARY_DIR}/assets")
    add_custom_target(create_asset_dir
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/assets"
        COMMENT "creating asset directory")
    add_dependencies(run_sandbox_dab create_asset_dir)
endif()
add_dependencies(dry_sandbox run_sandbox_dab)