cmake_minimum_required(VERSION 3.5)
project(GLESv1_CM LANGUAGES C)

# cglm
if(NOT TARGET cglm)
set(GLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/cglm)
set(CGLM_SHARED OFF CACHE BOOL "Shared option" FORCE)
set(CGLM_STATIC ON CACHE BOOL "Static option" FORCE)
add_subdirectory(${GLES_DIR})
target_compile_options(cglm PRIVATE -mno-sse)
endif()

# swizzle
add_library(swizzle STATIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/swizzle/swizzle.c)
target_include_directories(swizzle PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/swizzle)

# xgu
add_library(xgu INTERFACE)
target_include_directories(xgu INTERFACE ${GLES_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/lib/xgu)

# stb
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/stb)

file(GLOB GLESV1_CM_SOURCES
    "*.c"
)

add_library(GLESv1_CM
    ${GLESV1_CM_SOURCES}
)

target_include_directories(GLESv1_CM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(GLESv1_CM PRIVATE cglm swizzle xgu stb)
