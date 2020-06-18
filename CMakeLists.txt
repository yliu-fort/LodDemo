cmake_minimum_required (VERSION 2.8)
cmake_policy(VERSION 2.8)

project (LODDemo)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules")

link_directories(${CMAKE_SOURCE_DIR}/lib)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

### SET SOURCE PATH (todo: deprecate)
set(CONFIGURE_SRC_PATH_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/cmake_source_dir.h.in")
message(STATUS "CONFIGURE_SRC_PATH_TEMPLATE at ${CMAKE_CURRENT_SOURCE_DIR}/cmake_source_dir.h.in")

### GLOBAL CONFIGURATIONS
#First make sure you have CMake, Git, and GCC by typing as root (sudo) apt-get install g++ cmake git
#and then get the required packages: Using root (sudo) and type apt-get install libsoil-dev libglm-dev libassimp-dev libglew-dev libglfw3-dev libxinerama-dev libxcursor-dev libxi-dev
# find the required packages

add_subdirectory(external/glfw)
include_directories("external/glfw/include")

if(WIN32)
  set(LIBS glfw opengl32) # glfw3 if use binary libs
elseif(UNIX AND NOT APPLE)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
  find_package(OpenGL REQUIRED)
  add_definitions(${OPENGL_DEFINITIONS})
  find_package(X11 REQUIRED)
  # note that the order is important for setting the libs
  # use pkg-config --libs $(pkg-config --print-requires --print-requires-private glfw3) in a terminal to confirm
  set(LIBS ${GLFW3_LIBRARY} X11 Xrandr Xinerama Xi Xxf86vm Xcursor GL dl pthread)
  set (CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")
elseif(APPLE)
  INCLUDE_DIRECTORIES(/System/Library/Frameworks)
  FIND_LIBRARY(COCOA_LIBRARY Cocoa)
  FIND_LIBRARY(OpenGL_LIBRARY OpenGL)
  FIND_LIBRARY(IOKit_LIBRARY IOKit)
  FIND_LIBRARY(CoreVideo_LIBRARY CoreVideo)
  MARK_AS_ADVANCED(COCOA_LIBRARY OpenGL_LIBRARY)
  SET(APPLE_LIBS ${COCOA_LIBRARY} ${IOKit_LIBRARY} ${OpenGL_LIBRARY} ${CoreVideo_LIBRARY})
  SET(APPLE_LIBS ${APPLE_LIBS} ${GLFW3_LIBRARY})
  set(LIBS ${LIBS} ${APPLE_LIBS})
else()
  set(LIBS )
endif(WIN32)

# external libraries
include_directories("external")

# GLAD LIB
add_library(GLAD "external/glad.c")
set(LIBS ${LIBS} GLAD)

# TIFF LIB
#include_directories("external/tiff")
#set(LIBS ${LIBS} tiff)

# IMAGE_IO LIB
include_directories("external/stb")
add_library(STB_IMAGE "external/stb/stb_image.cpp")
set(LIBS ${LIBS} STB_IMAGE)

# GUI LIB
# Copy imgui_impl_glfw & imgui_impl_opengl3 out to root folder first
include_directories("external/imgui-1.74")
include_directories("external/imgui-1.74/examples")
file(GLOB IMGUI_SOURCES "external/imgui-1.74/*.cpp"
    "external/imgui-1.74/examples/imgui_impl_glfw.cpp"
    "external/imgui-1.74/examples/imgui_impl_opengl3.cpp")
add_library(IMGUI ${IMGUI_SOURCES})
set(LIBS ${LIBS} IMGUI)

# MY UTILITY LIB
include_directories("utility")
file(GLOB UTILITY_SOURCES "utility/*" "utility/glsl/*")
add_library(UTILITY ${UTILITY_SOURCES})
target_link_libraries(UTILITY IMGUI)
set(LIBS ${LIBS} UTILITY)
add_subdirectory("utility") # Provide absolute path for shaders

### CASE CONFIGURATION
#add_subdirectory("core/lod")
#add_subdirectory("core/surface")
#add_subdirectory("core/icosphere")
#add_subdirectory("core/planet")
#add_subdirectory("core/quaternion")
#add_subdirectory("core/quadtree")
add_subdirectory("core/quadtreeManaged")
add_subdirectory("core/sphericalLandscape")
add_subdirectory("core/textureFetch")
#add_subdirectory("core/mipmap")
### TESTS&DEMOS
#add_subdirectory("test")
