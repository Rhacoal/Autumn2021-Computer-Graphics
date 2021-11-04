cmake_minimum_required(VERSION 3.17)
project(assignment)

set(CMAKE_CXX_STANDARD 20)
if (APPLE)
    set(FRAMEWORKS
            "-framework Cocoa"
            "-framework IOKit"
            "-framework OpenGL"
            "-framework CoreVideo"
            "-framework OpenCL"
            )
endif ()

find_package(OpenGL REQUIRED)

# if CUDA is detected, use CUDA OpenCL
find_package(CUDAToolkit)
if (CUDAToolkit_FOUND)
    set(OpenCL_LIB CUDA::OpenCL)
    include_directories(${CUDAToolkit_INCLUDE_DIR})
else ()
    find_package(OpenCL REQUIRED)
    set(OpenCL_LIB OpenCL::OpenCL)
    include_directories(${OpenCL_INCLUDE_DIRS})
endif ()

if (NOT ${OpenCL_Found})
    message(FATAL_ERROR "OpenCL not found")
endif ()

if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Please select another Build Directory ! (and give it a clever name, like bin_Visual2012_64bits/)")
endif ()
if (CMAKE_SOURCE_DIR MATCHES " ")
    message("Your Source Directory contains spaces. If you experience problems when compiling, this can be the cause.")
endif ()
if (CMAKE_BINARY_DIR MATCHES " ")
    message("Your Build Directory contains spaces. If you experience problems when compiling, this can be the cause.")
endif ()


# Compile external dependencies
add_subdirectory(external)

# On Visual 2005 and above, this module can set the debug working directory
#cmake_policy(SET CMP0026 OLD)
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/rpavlik-cmake-modules-fe2273")
#include(CreateLaunchers)
#include(MSVCMultipleProcessCompile) # /MP

if (INCLUDE_DISTRIB)
    add_subdirectory(distrib)
endif (INCLUDE_DISTRIB)


include_directories(
        external/glfw-3.1.2/include/
        external/glm-0.9.7.1/
        external/glad/include/
        external/headeronly/
        external/tinydialog-3.8.8/
        include/
        .
)

set(ALL_LIBS
        ${OPENGL_LIBRARY}
        glad
        glfw
        imgui
        tinydialog
        ${OpenCL_LIB}
        ${FRAMEWORKS}
        )

add_definitions(
        -DTW_STATIC
        -DTW_NO_LIB_PRAGMA
        -DTW_NO_DIRECT3D
        -D_CRT_SECURE_NO_WARNINGS
        -DGL_SILENCE_DEPRECATION
)

add_executable(assignment
        lib/application.cpp
        lib/helper/axis_helper.cpp
        lib/importer/obj_loader.cpp
        lib/mesh/geometry.cpp
        lib/mesh/material.cpp
        lib/mesh/mesh.cpp
        lib/raytracing/rt.cpp
        lib/scene3d/object3d.cpp
        lib/scene3d/renderer.cpp
        lib/shaders/raytracing.cpp
        lib/stb_image_impl.cpp
        lib/util/shader.cpp
        lib/util/texture.cpp
        src/appmain.cpp
        lib/shaders/rt_common.h)
target_link_libraries(assignment
        ${ALL_LIBS}
        )
# Xcode and Visual working directories
set_target_properties(assignment PROPERTIES XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/")
#create_target_launcher(assignment WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/")

add_custom_target(copy_assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets
        )
add_custom_target(copy_shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/lib/shaders ${CMAKE_CURRENT_BINARY_DIR}/lib/shaders
        )
add_dependencies(assignment copy_assets copy_shaders)
