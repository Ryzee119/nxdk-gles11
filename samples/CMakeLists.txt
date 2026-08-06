cmake_minimum_required(VERSION 3.10)
project(nxdk-gles11-samples C)

set(NXDK_GLES11_WITH_GL4ES ON CACHE BOOL "" FORCE)
add_subdirectory(../ nxdk-gles11) # Pull in nxdk-gles11

add_executable(triangle_gles11 triangle_gles11.c)
target_link_libraries(triangle_gles11 PRIVATE GLESv1_CM ${NXDK_DIR}/lib/libpbkit.lib)

if(NXDK_GLES11_WITH_GL4ES)
    add_executable(triangle_gl4es triangle_gl4es.c)
    target_link_libraries(triangle_gl4es PRIVATE GL ${NXDK_DIR}/lib/libpbkit.lib)
endif()
