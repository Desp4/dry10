include(FetchContent)

function(fetch_tinygltf)
    if (TARGET tinygltf)
        return()
    endif()

    set(TINYGLTF_REPO https://github.com/syoyo/tinygltf.git)
    set(TINYGLTF_VER v2.5.0)

    # no package search, maybe a TODO
    message("-- downloading tinygltf ${TINYGLTF_VER}")
    FetchContent_Declare(DEP_TINYGLTF
        GIT_REPOSITORY ${TINYGLTF_REPO}
        GIT_TAG ${TINYGLTF_VER})

    if(NOT DEP_TINYGLTF_POPULATED)
        FetchContent_Populate(DEP_TINYGLTF)
    endif()
    add_library(tinygltf INTERFACE)
    target_include_directories(tinygltf INTERFACE ${dep_tinygltf_SOURCE_DIR})
endfunction()

function(fetch_libzip)
    if (TARGET zip)
        return()
    endif()

    if (NOT TARGET zlibstatic)
        set(ZLIB_REPO https://github.com/madler/zlib.git)
        set(ZLIB_VER v1.2.11)

        message("-- downloading zlib ${ZLIB_VER}")
        FetchContent_Declare(DEP_ZLIB
            GIT_REPOSITORY ${ZLIB_REPO}
            GIT_TAG ${ZLIB_VER})
        FetchContent_MakeAvailable(DEP_ZLIB)

        set(ZLIB_INC ${dep_zlib_SOURCE_DIR} ${dep_zlib_BINARY_DIR})
        target_include_directories(zlibstatic PUBLIC ${ZLIB_INC})
    endif()

    set(LIBZIP_REPO https://github.com/nih-at/libzip.git)
    set(LIBZIP_VER v1.7.3)

    set(ENABLE_BZIP2 OFF CACHE INTERNAL "Internal dependency option" FORCE)
    set(ENABLE_LZMA OFF CACHE INTERNAL "Internal dependency option" FORCE)
    set(BUILD_TOOLS OFF CACHE INTERNAL "Internal dependency option" FORCE)
    set(BUILD_REGRESS OFF CACHE INTERNAL "Internal dependency option" FORCE)
    set(BUILD_EXAMPLES OFF CACHE INTERNAL "Internal dependency option" FORCE)
    set(BUILD_DOC OFF CACHE INTERNAL "Internal dependency option" FORCE)
    set(LIBZIP_DO_INSTALL OFF CACHE INTERNAL "Internal dependency option" FORCE)

    # supress FindZLIB, make alias, export so that libzip exports don't fail
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
    export(TARGETS zlibstatic FILE "${CMAKE_CURRENT_BINARY_DIR}/zlibstatic-targets.cmake")
    # replace find module with a dummy, later revert cmake module path
    set(PREV_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/hacks)

    message("-- downloading libzip ${LIBZIP_VER}")
    FetchContent_Declare(DEP_LIBZIP
        GIT_REPOSITORY ${LIBZIP_REPO}
        GIT_TAG ${LIBZIP_VER})
    FetchContent_MakeAvailable(DEP_LIBZIP)

    set(CMAKE_MODULE_PATH ${PREV_CMAKE_MODULE_PATH})
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

    set(GLFW_REPO https://github.com/glfw/glfw.git)
    set(GLFW_VER 3.3.4)

    # settings
    set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Internal dependency option" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Internal dependency option" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Internal dependency option" FORCE)
    set(GLFW_INSTALL OFF CACHE INTERNAL "Internal dependency option" FORCE)

    message("-- downloading glfw ${GLFW_VER}")
    FetchContent_Declare(DEP_GLFW
        GIT_REPOSITORY ${GLFW_REPO}
        GIT_TAG ${GLFW_VER})
    FetchContent_MakeAvailable(DEP_GLFW)

    # link
    target_link_libraries(glfw PUBLIC ${GLFW_LIBRARIES})
    target_include_directories(glfw PUBLIC "${DEP_GLFW_SOURCE_DIR}/include")
endfunction()

function(fetch_glm)
    if (TARGET glm)
        return()
    endif()

    set(GLM_REPO https://github.com/g-truc/glm.git)
    set(GLM_VER 0.9.9.8)

    message("-- downloading glm ${GLM_VER}")
    FetchContent_Declare(DEP_GLM
        GIT_REPOSITORY ${GLM_REPO}
        GIT_TAG ${GLM_VER})
    FetchContent_MakeAvailable(DEP_GLM)  
endfunction()

function(fetch_shaderc)
    if (TARGET shaderc)
        return()
    endif()

    if (NOT TARGET SPIRV-Tools)
        if (NOT SPIRV-Headers_SOURCE_DIR) # NOTE : here not a target but a variable
            set(SPIRV_HEADERS_REPO https://github.com/KhronosGroup/SPIRV-Headers.git)
            set(SPIRV_HEADERS_HASH 07f259e68af3a540038fa32df522554e74f53ed5)
            # TODO : installation should work too, need to dance to make it work with spirv tools
            message("-- downloading SPIRV-Headers")

            FetchContent_Declare(DEP_SPIRV_HEADERS
                GIT_REPOSITORY ${SPIRV_HEADERS_REPO}
                URL_HASH ${SPIRV_HEADERS_HASH})

            if(NOT DEP_SPIRV_HEADERS_POPULATED)
                FetchContent_Populate(DEP_SPIRV_HEADERS)               
            endif()
            set(SPIRV-Headers_SOURCE_DIR ${dep_spirv_headers_SOURCE_DIR})
        endif()

        set(SPIRV_TOOLS_REPO https://github.com/KhronosGroup/SPIRV-Tools.git)
        set(SPIRV_TOOLS_HASH 5dd2f76918bb2d0d67628e338f60f724f3e02e13)

        # settings
        set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE INTERNAL "Internal dependency option" FORCE)
        set(SPIRV_SKIP_EXECUTABLES ON CACHE INTERNAL "Internal dependency option" FORCE)
        set(SPIRV_SKIP_TESTS ON CACHE INTERNAL "Internal dependency option" FORCE)

        message("-- downloading SPIRV-Tools ${SPIRV_TOOLS_VER}")
        FetchContent_Declare(DEP_SPIRV_TOOLS
            GIT_REPOSITORY ${SPIRV_TOOLS_REPO}
            URL_HASH ${SPIRV_TOOLS_HASH})
        FetchContent_MakeAvailable(DEP_SPIRV_TOOLS)
    endif()

    if (NOT TARGET glslang)
        set(GLSLANG_REPO https://github.com/KhronosGroup/glslang.git)
        set(GLSLANG_VER 11.4.0)

        # settings
        message("-- downloading glslang ${GLSLANG_VER}")
        set(SKIP_GLSLANG_INSTALL ON CACHE INTERNAL "Internal dependency option" FORCE)
        set(ENABLE_HLSL ON CACHE INTERNAL "Internal dependency option" FORCE) # required for shaderc
        set(ENABLE_CTEST OFF CACHE INTERNAL "Internal dependency option" FORCE)

        FetchContent_Declare(DEP_GLSLANG
            GIT_REPOSITORY ${GLSLANG_REPO}
            GIT_TAG ${GLSLANG_VER})
        FetchContent_MakeAvailable(DEP_GLSLANG) 
    endif()

    set(SHADERC_REPO https://github.com/google/shaderc.git)
    set(SHADERC_VER v2021.0)

    set(SHADERC_SKIP_INSTALL ON CACHE INTERNAL "Internal dependency option" FORCE)
    set(SHADERC_SKIP_TESTS ON CACHE INTERNAL "Internal dependency option" FORCE)
    set(SHADERC_SKIP_EXAMPLES ON CACHE INTERNAL "Internal dependency option" FORCE)
    set(SHADERC_ENABLE_SHARED_CRT ON CACHE INTERNAL "Internal dependency option" FORCE)

    message("-- downloading shaderc ${SHADERC_VER}")
    FetchContent_Declare(DEP_SHADERC
        GIT_REPOSITORY ${SHADERC_REPO}
        GIT_TAG ${SHADERC_VER})
    FetchContent_MakeAvailable(DEP_SHADERC)
endfunction()

function(fetch_spirv_cross)
    if (TARGET spirv-cross-core)
        return()
    endif()

    set(SPIRV_CROSS_REPO https://github.com/KhronosGroup/SPIRV-Cross.git)
    set(SPIRV_CROSS_VER 2021-01-15)

    # settings
    set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE INTERNAL "Internal dependency option" FORCE)
    set(SPIRV_CROSS_SKIP_INSTALL ON CACHE INTERNAL "Internal dependency option" FORCE)

    FetchContent_Declare(DEP_SPIRV_CROSS
        GIT_REPOSITORY ${SPIRV_CROSS_REPO}
        GIT_TAG ${SPIRV_CROSS_VER})
    FetchContent_MakeAvailable(DEP_SPIRV_CROSS)
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

function(fetch_all_deps)
    set(SKIP_INSTALL_ALL OFF CACHE INTERNAL "Internal dependency option" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Internal dependency option" FORCE)

    if (DRY_BUILD_DAB)
        fetch_tinygltf()
        fetch_stbi()
        fetch_libzip()
        fetch_shaderc()
    endif()

    if (DRY_BUILD_DRY1)
        fetch_vulkan()
        fetch_glfw()
        fetch_glm()
        fetch_libzip()
        fetch_stbi()
        fetch_spirv_cross()
    endif()
endfunction()