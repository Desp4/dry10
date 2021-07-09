cmake_minimum_required(VERSION 3.12)

project(dry_tests)

set(CMAKE_CXX_STANDARD 20)

add_executable(orbitals "${PROJECT_SOURCE_DIR}/src/orbitals.cpp")
add_executable(mtf "${PROJECT_SOURCE_DIR}/src/mtf.cpp")

target_link_libraries(orbitals PRIVATE dry1)
target_link_libraries(mtf PRIVATE dry1)

add_custom_target(run_dab
    COMMAND dab "${PROJECT_SOURCE_DIR}/assets" "${PROJECT_BINARY_DIR}/assets/assets.zip"
    COMMENT "archiving assets for sandbox")

if (NOT EXISTS "${PROJECT_BINARY_DIR}/assets")
    add_custom_target(create_asset_dir
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/assets"
        COMMENT "creating asset directory")
    add_dependencies(run_dab create_asset_dir)
endif()

add_dependencies(orbitals run_dab)
add_dependencies(mtf run_dab)