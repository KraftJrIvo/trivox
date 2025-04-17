cmake_minimum_required(VERSION 3.15...3.27)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(TRIVOX VERSION 1.0 DESCRIPTION "raytraced oct-tree voxel 3d engine" LANGUAGES CXX C)

# TARGETS
add_executable(embedfile src/embedfile.c)
add_custom_command(OUTPUT res_icon.c          COMMAND embedfile res_icon          ${TRIVOX_SOURCE_DIR}/res/icon.png      EPENDS ${TRIVOX_SOURCE_DIR}/res/icon.png)
add_custom_command(OUTPUT res_raytrace_frag.c COMMAND embedfile res_raytrace_frag ${TRIVOX_SOURCE_DIR}/res/raytrace.frag DEPENDS ${TRIVOX_SOURCE_DIR}/res/raytrace.frag)

set(TRIVOX_SOURCE_FILES
    "src/memory.hpp"
    "src/types.hpp"
    "src/world.cpp"
    "src/render.cpp"
    "src/main.cpp"
    res_icon.c
    res_raytrace_frag.c
    "res/icon.rc"
)
add_executable(TRIVOX ${TRIVOX_SOURCE_FILES})
target_include_directories(TRIVOX PUBLIC "src")

# DEPENDENCIES
set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
    include(FetchContent)
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
target_compile_options(raylib PUBLIC -DGRAPHICS_API_OPENGL_33)
target_link_libraries(TRIVOX PUBLIC raylib)

add_custom_target(prepare_rel
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/Release
)
add_dependencies(raylib prepare_rel)

add_custom_target(copy_assets_rel
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/res ${CMAKE_CURRENT_BINARY_DIR}/Release/res
)
add_dependencies(raylib copy_assets_rel)

add_custom_target(prepare_deb
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/Debug
)
add_dependencies(raylib prepare_deb)

add_custom_target(copy_assets_deb
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/res ${CMAKE_CURRENT_BINARY_DIR}/Debug/res
)
add_dependencies(raylib copy_assets_deb)