cmake_minimum_required(VERSION 3.12)

project(dab)
set(CMAKE_CXX_STANDARD 20)

add_library(dablib INTERFACE)
target_include_directories(dablib INTERFACE "${PROJECT_SOURCE_DIR}/include")

if (DRY_BUILD_DAB)
    set(DAB_SOURCES
        "${PROJECT_SOURCE_DIR}/src//dab.cpp"
        "${PROJECT_SOURCE_DIR}/src/dab_impl.cpp"
        "${PROJECT_SOURCE_DIR}/src/shader_import.cpp"
        "${PROJECT_SOURCE_DIR}/src/mesh_import.cpp"
        "${PROJECT_SOURCE_DIR}/src/tex_import.cpp"
        "${PROJECT_SOURCE_DIR}/src/lib_impls.cpp")

    add_executable(dab ${DAB_SOURCES})

    target_link_libraries(dab PUBLIC stbi shaderc tinygltf dry_common)
    target_include_directories(dab PUBLIC "${PROJECT_SOURCE_DIR}/include")
endif()
