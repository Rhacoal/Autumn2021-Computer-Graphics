cmake_minimum_required(VERSION 3.17)
project(assignment)

set(CMAKE_CXX_STANDARD 20)
if (APPLE)
    set(FRAMEWORKS
            "-framework Cocoa"
            "-framework IOKit"
            "-framework OpenGL"
            "-framework CoreVideo"
            )
endif ()

find_package(OpenGL REQUIRED)

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
cmake_policy(SET CMP0026 OLD)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/rpavlik-cmake-modules-fe2273")
include(CreateLaunchers)
include(MSVCMultipleProcessCompile) # /MP

if (INCLUDE_DISTRIB)
    add_subdirectory(distrib)
endif (INCLUDE_DISTRIB)


include_directories(
        external/AntTweakBar-1.16/include/
        external/glfw-3.1.2/include/
        external/glm-0.9.7.1/
        external/glad/include/
        include/
        .
)

set(ALL_LIBS
        ${OPENGL_LIBRARY}
        glad
        glfw
        ${FRAMEWORKS}
        )

add_definitions(
        -DTW_STATIC
        -DTW_NO_LIB_PRAGMA
        -DTW_NO_DIRECT3D
        -D_CRT_SECURE_NO_WARNINGS
        -DGL_SILENCE_DEPRECATION
)

add_executable(assignment lib/scene3d/object3d.cpp lib/mesh/mesh.cpp lib/application.cpp lib/mesh/geometry.cpp lib/stb_image_impl.cpp lib/mesh/material.cpp lib/scene3d/renderer.cpp lib/shader/shader.cpp src/appmain.cpp)
target_link_libraries(assignment
        ${ALL_LIBS}
        )
# Xcode and Visual working directories
set_target_properties(assignment PROPERTIES XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/")
create_target_launcher(assignment WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/")

configure_file(lib/shader/phong.fsh lib/shader/phong.fsh COPYONLY)
configure_file(lib/shader/phong.vsh lib/shader/phong.vsh COPYONLY)

configure_file(lib/shader/basic.fsh lib/shader/basic.fsh COPYONLY)
configure_file(lib/shader/basic.vsh lib/shader/basic.vsh COPYONLY)
