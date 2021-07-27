function(fetch_tinygltf)
    if (TARGET tinygltf)
        return()
    endif()

    add_library(tinygltf INTERFACE)
    target_include_directories(tinygltf INTERFACE external/tinygltf)
endfunction()

function(fetch_stbi)
    if (TARGET stbi)
        return()
    endif()

    fetch_tinygltf()
    add_library(stbi ALIAS tinygltf)
endfunction()

function(fetch_glfw)
    if (TARGET glfw)
        return()
    endif()

    # settings
    set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Internal dependency option" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Internal dependency option" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Internal dependency option" FORCE)
    set(GLFW_INSTALL OFF CACHE INTERNAL "Internal dependency option" FORCE)

    add_subdirectory(external/glfw ${PROJECT_BINARY_DIR}/external/glfw)

    # link
    target_link_libraries(glfw PUBLIC ${GLFW_LIBRARIES})
    target_include_directories(glfw PUBLIC external/glfw/include)
endfunction()

function(fetch_glm)
    if (TARGET glm)
        return()
    endif()

    add_subdirectory(external/glm ${PROJECT_BINARY_DIR}/external/glm)
endfunction()

function(fetch_shaderc)
    if (TARGET shaderc)
        return()
    endif()

    if (NOT TARGET SPIRV-Tools)
        if (NOT SPIRV-Headers_SOURCE_DIR) # NOTE : here not a target but a variable
            set(SPIRV-Headers_SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/spirv-headers)
        endif()

        # settings
        set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE INTERNAL "Internal dependency option" FORCE)
        set(SPIRV_SKIP_EXECUTABLES ON CACHE INTERNAL "Internal dependency option" FORCE)
        set(SPIRV_SKIP_TESTS ON CACHE INTERNAL "Internal dependency option" FORCE)

        add_subdirectory(external/spirv-tools ${PROJECT_BINARY_DIR}/external/spirv-tools)
    endif()

    if (NOT TARGET glslang)
        # settings
        set(SKIP_GLSLANG_INSTALL ON CACHE INTERNAL "Internal dependency option" FORCE)
        set(ENABLE_HLSL ON CACHE INTERNAL "Internal dependency option" FORCE) # required for shaderc
        set(ENABLE_CTEST OFF CACHE INTERNAL "Internal dependency option" FORCE)

        add_subdirectory(external/glslang ${PROJECT_BINARY_DIR}/external/glslang)
    endif()

    # settings
    set(SHADERC_SKIP_INSTALL ON CACHE INTERNAL "Internal dependency option" FORCE)
    set(SHADERC_SKIP_TESTS ON CACHE INTERNAL "Internal dependency option" FORCE)
    set(SHADERC_SKIP_EXAMPLES ON CACHE INTERNAL "Internal dependency option" FORCE)
    set(SHADERC_ENABLE_SHARED_CRT ON CACHE INTERNAL "Internal dependency option" FORCE)

    add_subdirectory(external/shaderc ${PROJECT_BINARY_DIR}/external/shaderc)
endfunction()

function(fetch_spirv_cross)
    if (TARGET spirv-cross-core)
        return()
    endif()

    # settings
    set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE INTERNAL "Internal dependency option" FORCE)
    set(SPIRV_CROSS_SKIP_INSTALL ON CACHE INTERNAL "Internal dependency option" FORCE)

    add_subdirectory(external/spirv-cross ${PROJECT_BINARY_DIR}/external/spirv-cross)
endfunction()

function(fetch_vulkan)
    if (NOT TARGET vulkan)
        if (DEFINED ENV{VULKAN_SDK} AND WIN32)
            set(VULKAN_INCLUDE "$ENV{VULKAN_SDK}/Include")

            find_library(VULKAN_LIBRARY NAMES vulkan-1 HINTS "$ENV{VULKAN_SDK}/Lib")
            add_library(vulkan UNKNOWN IMPORTED)
            set_target_properties(vulkan PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${VULKAN_INCLUDE})
            set_target_properties(vulkan PROPERTIES IMPORTED_LOCATION ${VULKAN_LIBRARY})
        else()
            message(FATAL_ERROR "-- Can't do chief, use windows and vulkan sdk")
        endif()
    endif()
endfunction()

function(fetch_vma)
    if (TARGET vma)
        return()
    endif()

    # NOTE : for static linking need alias, so need vulkan as a target first
    fetch_vulkan()

    # NOTE : force static
    set(VMA_STATIC_VULKAN_FUNCTIONS ON CACHE INTERNAL "Internal dependency option" FORCE)
    set(VMA_DYNAMIC_VULKAN_FUNCTIONS OFF CACHE INTERNAL "Internal dependency option" FORCE)

    add_library(Vulkan::Vulkan ALIAS vulkan)

    add_subdirectory(external/vma ${PROJECT_BINARY_DIR}/external/vma)
    add_library(vma ALIAS VulkanMemoryAllocator)
endfunction()

function(fetch_all_deps)
    set(SKIP_INSTALL_ALL OFF CACHE INTERNAL "Internal dependency option" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Internal dependency option" FORCE)

    if (DRY_BUILD_DAB)
        fetch_tinygltf()
        fetch_stbi()
        fetch_shaderc()
    endif()

    if (DRY_BUILD_DRY1)
        fetch_vulkan()
        fetch_glfw()
        fetch_glm()
        fetch_stbi()
        fetch_spirv_cross()
        fetch_vma()
    endif()
endfunction()