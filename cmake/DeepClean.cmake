cmake_minimum_required(VERSION 3.24)

# Run from the repository root: cmake -P cmake/DeepClean.cmake
get_filename_component(repo_root "${CMAKE_CURRENT_LIST_DIR}/.." REALPATH)
set(build_root "${repo_root}/build")

if(IS_SYMLINK "${build_root}")
    message(FATAL_ERROR "Refusing to remove a redirected build directory.")
endif()
if(NOT EXISTS "${build_root}")
    message(STATUS "Nothing to clean: build/ does not exist.")
    return()
endif()
if(NOT IS_DIRECTORY "${build_root}")
    message(FATAL_ERROR "Expected build/ to be a regular directory.")
endif()

# Resolve and verify the exact target before recursive deletion.
file(REAL_PATH "${build_root}" resolved_build_root)
if(NOT resolved_build_root STREQUAL build_root)
    message(FATAL_ERROR "Refusing to remove a redirected build directory.")
endif()
file(REMOVE_RECURSE "${build_root}")
if(EXISTS "${build_root}")
    message(FATAL_ERROR "Could not remove build/. Stop the app/debugger and any builds, then try again.")
endif()

message(STATUS "Removed build/, including all downloaded and compiled dependencies.")
message(STATUS "Next configure downloads FLTK again; the next build recompiles it.")
