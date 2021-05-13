include(FetchContent)

function(fetch_all_deps)

    # === 1. assimp, also zlib ===
    set(ASSIMP_REPO https://github.com/assimp/assimp.git)
    set(ASSIMP_VER 5.0.1)

    # settings
    set(SKIP_INSTALL_ALL OFF CACHE INTERNAL "Internal dependency option" FORCE)

    find_package(assimp ${ASSIMP_VER} QUIET)
    if(NOT assimp_FOUND)
        message("-- assimp ${ASSIMP_VER} installation not found, downloading")

        set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Internal dependency option" FORCE)
        set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE INTERNAL "Internal dependency option" FORCE)
        set(ASSIMP_BUILD_TESTS OFF CACHE INTERNAL "Internal dependency option" FORCE)
        set(ASSIMP_INSTALL OFF CACHE INTERNAL "Internal dependency option" FORCE)
        set(ASSIMP_BUILD_ZLIB ON CACHE INTERNAL "Internal dependency option" FORCE)

        FetchContent_Declare(DEP_ASSIMP
            GIT_REPOSITORY ${ASSIMP_REPO}
            GIT_TAG v${ASSIMP_VER})
        FetchContent_MakeAvailable(DEP_ASSIMP)
        
        set(ZLIB_INC "${dep_assimp_SOURCE_DIR}/contrib/zlib" "${dep_assimp_BINARY_DIR}/contrib/zlib")
    else() # then build zlib yourself
        set(ZLIB_REPO https://github.com/madler/zlib.git)
        set(ZLIB_VER 1.2.11)

        message("-- downloading zlib ${ZLIB_VER}")
        FetchContent_Declare(DEP_ZLIB
            GIT_REPOSITORY ${ZLIB_REPO}
            GIT_TAG v${ZLIB_VER})
        FetchContent_MakeAvailable(DEP_ZLIB)

        set(ZLIB_INC ${dep_zlib_SOURCE_DIR} ${dep_zlib_BINARY_DIR})
    endif()

    # add headers, zconf and zlib are in build and src dirs
    target_include_directories(zlibstatic PUBLIC ${ZLIB_INC})
    # supress FindZLIB, make alias, export so that libzip exports don't fail
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
    export(TARGETS zlibstatic FILE "${CMAKE_CURRENT_BINARY_DIR}/zlibstatic-targets.cmake")

    # === 2. libzip ===
    set(LIBZIP_REPO https://github.com/nih-at/libzip.git)
    set(LIBZIP_VER 1.7.3)

    find_package(libzip ${LIBZIP_VER} QUIET)
    if(NOT libzip_FOUND)
        message("-- libzip ${LIBZIP_VER} installation not found, downloading")

        # settings
        set(ENABLE_BZIP2 OFF CACHE INTERNAL "Internal dependency option" FORCE)
        set(ENABLE_LZMA OFF CACHE INTERNAL "Internal dependency option" FORCE)
        set(BUILD_TOOLS OFF CACHE INTERNAL "Internal dependency option" FORCE)
        set(BUILD_REGRESS OFF CACHE INTERNAL "Internal dependency option" FORCE)
        set(BUILD_EXAMPLES OFF CACHE INTERNAL "Internal dependency option" FORCE)
        set(BUILD_DOC OFF CACHE INTERNAL "Internal dependency option" FORCE)
        set(LIBZIP_DO_INSTALL OFF CACHE INTERNAL "Internal dependency option" FORCE)

        FetchContent_Declare(DEP_LIBZIP
            GIT_REPOSITORY ${LIBZIP_REPO}
            GIT_TAG v${LIBZIP_VER})
        FetchContent_MakeAvailable(DEP_LIBZIP)
    endif()

    # === 3. stb ===
    set(STB_REPO https://github.com/nothings/stb.git)

    message("-- downloading stb")
    FetchContent_Declare(DEP_STB GIT_REPOSITORY ${STB_REPO})
    if(NOT DEP_STB_POPULATED)
        FetchContent_Populate(DEP_STB)

        add_library(stb INTERFACE)
        target_include_directories(stb INTERFACE ${dep_stb_SOURCE_DIR})
    endif()

    # === 4. glfw ===
    set(GLFW_REPO https://github.com/glfw/glfw.git)
    set(GLFW_VER 3.3.4)

    find_package(glfw3 ${GLFW_VER} QUIET)
    if(NOT glfw3_FOUND)
        message("-- glfw ${GLFW_VER} installation not found, downloading")

        # settings
        set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Internal dependency option" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Internal dependency option" FORCE)
        set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Internal dependency option" FORCE)
        set(GLFW_INSTALL OFF CACHE INTERNAL "Internal dependency option" FORCE)

        FetchContent_Declare(DEP_GLFW
            GIT_REPOSITORY ${GLFW_REPO}
            GIT_TAG ${GLFW_VER})
        FetchContent_MakeAvailable(DEP_GLFW)
        # link
        target_link_libraries(glfw PUBLIC ${GLFW_LIBRARIES})
        target_include_directories(glfw PUBLIC "${DEP_GLFW_SOURCE_DIR}/include")
    endif()

    # === 5. glm ===
    set(GLM_REPO https://github.com/g-truc/glm.git)
    set(GLM_VER 0.9.9.8)

    find_package(glm ${GLM_VER} QUIET)
    if(NOT glm_FOUND)
        message("-- glm ${GLM_VER} installation not found, downloading")

        FetchContent_Declare(DEP_GLM
            GIT_REPOSITORY ${GLM_REPO}
            GIT_TAG ${GLM_VER})
        FetchContent_MakeAvailable(DEP_GLM)
    endif()

    # === 6. shaderc ===
    # looking for install, sdk version misses debug symbols at places

    set(SHADERC_REPO https://github.com/google/shaderc.git)
    set(SHADERC_VER v2021.0)
 
    find_package(shaderc QUIET)
    if(NOT shaderc_FOUND)
        message("-- shaderc installation not found, downloading")

        # === 6.1 spirv-headers ===
        set(SPIRV_HEADERS_REPO https://github.com/KhronosGroup/SPIRV-Headers.git)
        set(SPIRV_HEADERS_HASH 85b7e00c7d785962ffe851a177c84353d037dcb6)
        # TODO : installation should work too, need to dance to make it work with spirv tools
        message("-- downloading SPIRV-Headers")

        FetchContent_Declare(DEP_SPIRV_HEADERS
            GIT_REPOSITORY ${SPIRV_HEADERS_REPO}
            URL_HASH ${SPIRV_HEADERS_HASH})

        if(NOT DEP_SPIRV_HEADERS_POPULATED)
            FetchContent_Populate(DEP_SPIRV_HEADERS)
            set(SPIRV-Headers_SOURCE_DIR ${dep_spirv_headers_SOURCE_DIR})
        endif()

        # === 6.2 spirv-tools ===
        set(SPIRV_TOOLS_REPO https://github.com/KhronosGroup/SPIRV-Tools.git)
        set(SPIRV_TOOLS_VER v2021.1)

        find_package(SPIRV-Tools QUIET)
        if(NOT SPIRV-Tools_FOUND)
            message("-- SPIRV-Tools installation not found, downloading")

            # settings
            set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE INTERNAL "Internal dependency option" FORCE)
            set(SPIRV_SKIP_EXECUTABLES ON CACHE INTERNAL "Internal dependency option" FORCE)
            set(SPIRV_SKIP_TESTS ON CACHE INTERNAL "Internal dependency option" FORCE)

            FetchContent_Declare(DEP_SPIRV_TOOLS
                GIT_REPOSITORY ${SPIRV_TOOLS_REPO}
                GIT_TAG ${SPIRV_TOOLS_VER})
            FetchContent_MakeAvailable(DEP_SPIRV_TOOLS)
        endif()  

        # === 6.3 glslang ===
        set(GLSLANG_REPO https://github.com/KhronosGroup/glslang.git)
        set(GLSLANG_VER 11.4.0)

        find_package(glslang ${GLSLANG_VER} QUIET)
        if(NOT glslang_FOUND)
            message("-- glslang ${GLSLANG_VER} installation not found, downloading")
            
            # settings
            set(SKIP_GLSLANG_INSTALL ON CACHE INTERNAL "Internal dependency option" FORCE)
            set(ENABLE_HLSL ON CACHE INTERNAL "Internal dependency option" FORCE) # required for shaderc
            set(ENABLE_CTEST OFF CACHE INTERNAL "Internal dependency option" FORCE)

            FetchContent_Declare(DEP_GLSLANG
                GIT_REPOSITORY ${GLSLANG_REPO}
                GIT_TAG ${GLSLANG_VER})
            FetchContent_MakeAvailable(DEP_GLSLANG)
        endif()      

        # settings
        set(SHADERC_SKIP_INSTALL ON CACHE INTERNAL "Internal dependency option" FORCE)
        set(SHADERC_SKIP_TESTS ON CACHE INTERNAL "Internal dependency option" FORCE)
        set(SHADERC_SKIP_EXAMPLES ON CACHE INTERNAL "Internal dependency option" FORCE)
        set(SHADERC_ENABLE_SHARED_CRT ON CACHE INTERNAL "Internal dependency option" FORCE)

        FetchContent_Declare(DEP_SHADERC
            GIT_REPOSITORY ${SHADERC_REPO}
            GIT_TAG ${SHADERC_VER})
        FetchContent_MakeAvailable(DEP_SHADERC)
    endif()

    # === 7. spirv-cross ===
    # it to misses debug symbols
    set(SPIRV_CROSS_REPO https://github.com/KhronosGroup/SPIRV-Cross.git)
    set(SPIRV_CROSS_VER 2021-01-15)

    find_package(spirv-cross-core QUIET)
    if(NOT spirv-cross-core_FOUND)
        message("-- spirv-cross installation not found, downloading")

        # settings
        set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE INTERNAL "Internal dependency option" FORCE)
        set(SPIRV_CROSS_SKIP_INSTALL ON CACHE INTERNAL "Internal dependency option" FORCE)

        FetchContent_Declare(DEP_SPIRV_CROSS
            GIT_REPOSITORY ${SPIRV_CROSS_REPO}
            GIT_TAG ${SPIRV_CROSS_VER})
        FetchContent_MakeAvailable(DEP_SPIRV_CROSS)
    endif()

    # === 8. vulkan ===
    # need sdk
    find_package(vulkan QUIET)
    if(NOT TARGET vulkan)
        message(FATAL_ERROR "-- Vulkan lib not found, make sure Vulkan SDK is present")
    endif()
endfunction()