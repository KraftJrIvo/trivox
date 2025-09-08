cmake_minimum_required(VERSION 3.15...3.27)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(TRIVOX VERSION 1.0 DESCRIPTION "raytraced oct-tree voxel 3d engine" LANGUAGES CXX C)

# TARGETS
add_executable(embedfile src/embedfile.c)
add_custom_command(OUTPUT res_icon.c          COMMAND embedfile res_icon          ${TRIVOX_SOURCE_DIR}/res/icon.png      DEPENDS ${TRIVOX_SOURCE_DIR}/res/icon.png)
add_custom_command(OUTPUT res_raytrace_frag.c COMMAND embedfile res_raytrace_frag ${TRIVOX_SOURCE_DIR}/res/raytrace.frag DEPENDS ${TRIVOX_SOURCE_DIR}/res/raytrace.frag)
add_custom_command(OUTPUT res_res_casc_frag.c COMMAND embedfile res_res_casc_frag ${TRIVOX_SOURCE_DIR}/res/res_casc.frag DEPENDS ${TRIVOX_SOURCE_DIR}/res/res_casc.frag)

set(TRIVOX_SOURCE_FILES
    "src/memory.hpp"
    "src/pyramid.hpp"
    "src/world.h"
    "src/world.cpp"
    "src/shape.cpp"
    "src/entity.hpp"
    "src/render.cpp"
    "src/main.cpp"
    "src/ent/bounce_ball.cpp"
    "src/ent/rot_box.cpp"
    "src/ent/inf_point.cpp"
    res_icon.c
    res_raytrace_frag.c
    res_res_casc_frag.c
    "res/icon.rc"
)
add_executable(TRIVOX ${TRIVOX_SOURCE_FILES})
target_include_directories(TRIVOX PUBLIC "src")

# DEPENDENCIES
set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET)
if (NOT raylib_FOUND)
    include(FetchContent)
    set(CMAKE_TLS_VERIFY FALSE)
    FetchContent_Declare(
        raylib
        DOWNLOAD_EXTRACT_TIMESTAMP OFF
        URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    )
      FetchContent_GetProperties(raylib)
      if (NOT raylib_POPULATED)
          set(FETCHCONTENT_QUIET NO)
          set(CMAKE_POSITION_INDEPENDENT_CODE ON)
          FetchContent_MakeAvailable(raylib)
          set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
      endif()
endif()
target_compile_options(raylib PUBLIC -DRAYMATH_DISABLE_CPP_OPERATORS)
target_compile_options(raylib PUBLIC -DGRAPHICS_API_OPENGL_43)
target_link_libraries(TRIVOX PUBLIC raylib)

find_package(Eigen3 CONFIG QUIET)
if (NOT Eigen3_FOUND)
    include(FetchContent)
    set(CMAKE_TLS_VERIFY FALSE)
    FetchContent_Declare(
        Eigen
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE)
    set(BUILD_TESTING OFF)
    set(EIGEN_BUILD_TESTING OFF)
    set(EIGEN_MPL2_ONLY ON)
    set(EIGEN_BUILD_PKGCONFIG OFF)
    set(EIGEN_BUILD_DOC OFF)
    FetchContent_MakeAvailable(Eigen)
    list(POP_BACK CMAKE_MESSAGE_INDENT)
endif()
target_link_libraries(TRIVOX PRIVATE Eigen3::Eigen)