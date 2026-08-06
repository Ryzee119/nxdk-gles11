cmake_minimum_required(VERSION 3.10)
project(GLESv1_CM LANGUAGES C)

# swizzle
add_library(swizzle STATIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/swizzle/swizzle.c)
target_include_directories(swizzle PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/swizzle)

# xgu
add_library(xgu INTERFACE)
target_include_directories(xgu INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/xgu)

# stb
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/stb)

# arena
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/arena arena_build)
target_compile_definitions(arena PRIVATE ARENA_DISABLE_VSNPRINTF ARENA_ALIGNMENT_SIZE=31)

file(GLOB GLESV1_CM_SOURCES
    "*.c"
)

option(NXDK_GLES11_WITH_GL4ES "Build gl4es wrapper for Desktop OpenGL" OFF)

if(NXDK_GLES11_WITH_GL4ES)
    # Configure gl4es
    set(NOX11 ON CACHE BOOL "" FORCE)
    set(NO_GBM ON CACHE BOOL "" FORCE)
    set(NOEGL ON CACHE BOOL "" FORCE)
    set(STATICLIB ON CACHE BOOL "" FORCE)
    set(DEFAULT_ES 1 CACHE STRING "" FORCE)
    set(NO_LOADER ON CACHE BOOL "" FORCE)

    include(FetchContent)
    FetchContent_Declare(
        gl4es
        GIT_REPOSITORY https://github.com/ptitseb/gl4es.git
        GIT_TAG 81547d986798e876de8b434193920b606a72363f
    )
    FetchContent_MakeAvailable(gl4es)
    
    if(TARGET GL)
        target_compile_options(GL PRIVATE -w)
    endif()
    
    # Generate shim
    set(GLES11_GL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/GLES)
    set(GL_PREFIX_H ${CMAKE_CURRENT_BINARY_DIR}/gl_prefix.h)
    set(GL4ES_SHIM_C ${CMAKE_CURRENT_BINARY_DIR}/gl4es_shim.c)
    
    add_custom_command(
        OUTPUT ${GL_PREFIX_H} ${GL4ES_SHIM_C}
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_gl4es_shim.py ${CMAKE_CURRENT_SOURCE_DIR} ${GLES11_GL_DIR} ${GL_PREFIX_H} ${GL4ES_SHIM_C}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_gl4es_shim.py ${GLES11_GL_DIR}/gl.h
        COMMENT "Generating GL prefix header and shim"
    )
    
    list(APPEND GLESV1_CM_SOURCES ${GL4ES_SHIM_C})
endif()

add_library(GLESv1_CM
    ${GLESV1_CM_SOURCES}
)

if(NXDK_GLES11_WITH_GL4ES)
    target_compile_definitions(GLESv1_CM PUBLIC NXDK_GLES11_WITH_GL4ES)
    target_compile_definitions(GLESv1_CM PRIVATE NXDK_GLES11_PREFIX_EXPORTS)
    target_link_libraries(GLESv1_CM PUBLIC GL) # GL is the gl4es target
    
    # Make linking against GL transparently include GLESv1_CM
    if(TARGET GL)
        if(POLICY CMP0079)
            cmake_policy(SET CMP0079 NEW)
        endif()
        target_link_libraries(GL INTERFACE GLESv1_CM)
        # Also provide an OpenGL::GL alias which is common in CMake projects
        if (NOT TARGET OpenGL::GL)
            add_library(OpenGL::GL ALIAS GL)
        endif()
    endif()
endif()

target_include_directories(GLESv1_CM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(NXDK_GLES11_WITH_GL4ES)
    target_include_directories(GLESv1_CM PUBLIC 
        ${gl4es_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

target_link_libraries(GLESv1_CM PRIVATE swizzle xgu stb arena)
