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

add_library(GLESv1_CM
    ${GLESV1_CM_SOURCES}
)

target_include_directories(GLESv1_CM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(GLESv1_CM PRIVATE swizzle xgu stb arena)
