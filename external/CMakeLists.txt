if (MSVC AND NOT "${MSVC_VERSION}" LESS 1400)
    add_compile_options(/MP)
endif ()


add_definitions(
        -DTW_STATIC
        -DTW_NO_LIB_PRAGMA
        -DTW_NO_DIRECT3D
        -D_CRT_SECURE_NO_WARNINGS
)

### GLFW ###

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

include_directories(
        ${OPENGL_INCLUDE_DIRS}
)

add_subdirectory(glfw-3.3.4)

### GLAD ###
add_library(glad glad/include/glad/glad.h glad/src/glad.c headeronly/cl.hpp)
target_include_directories(glad PUBLIC glad/include/)


### IMGUI ###
file(GLOB IMGUI_SRC imgui-1.85/src/*.cpp)
file(GLOB IMGUI_INCLUDE imgui-1.85/include/*.h)
add_library(imgui
        ${IMGUI_SRC}
        ${IMGUI_INCLUDE})
target_include_directories(imgui PUBLIC imgui-1.85/include glfw-3.3.4/include)


### ASSIMP ###
# AssImp already has a CMakeLists.txt so let's use these

# Compile built-in, modified version of Zlib
include(CheckIncludeFile)
include(CheckTypeSize)
include(CheckFunctionExists)

# Compile without Boost
add_definitions(-DASSIMP_BUILD_BOOST_WORKAROUND)

# tinydialog
add_library(tinydialog tinydialog-3.8.8/tinyfiledialogs.cpp tinydialog-3.8.8/tinyfiledialogs.h)
target_include_directories(tinydialog PUBLIC tinydialog-3.8.8)

# Compile AssImp
#set(LIB_INSTALL_DIR "lib")
#set(ZLIB_LIBRARIES zlib)
set(BUILD_SHARED_LIBS OFF)
set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_BUILD_SAMPLES OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
add_subdirectory(assimp-5.0.1)

