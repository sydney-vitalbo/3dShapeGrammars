cmake_minimum_required(VERSION 3.24)

# Run from the repository root: cmake -P cmake/Clean.cmake
# This is a standalone script so it can also remove the CMake cache and
# generated build system without deleting a currently running build target.
get_filename_component(repo_root "${CMAKE_CURRENT_LIST_DIR}/.." REALPATH)
set(build_root "${repo_root}/build")

if(NOT EXISTS "${build_root}")
    message(STATUS "Nothing to clean: build/ does not exist.")
    return()
endif()
if(IS_SYMLINK "${build_root}" OR NOT IS_DIRECTORY "${build_root}")
    message(FATAL_ERROR "Expected build/ to be a regular directory.")
endif()
file(REAL_PATH "${build_root}" resolved_build_root)
if(NOT resolved_build_root STREQUAL build_root)
    message(FATAL_ERROR "Refusing to clean a redirected build directory.")
endif()

set(remove_paths)
file(GLOB entries LIST_DIRECTORIES TRUE "${build_root}/*")
foreach(entry IN LISTS entries)
    get_filename_component(name "${entry}" NAME)
    if(name STREQUAL "downloads")
        # Preserve manually cached dependency archives too.
        continue()
    endif()
    if(IS_DIRECTORY "${entry}" AND NOT IS_SYMLINK "${entry}"
            AND EXISTS "${entry}/_deps")
        file(GLOB contents LIST_DIRECTORIES TRUE "${entry}/*")
        foreach(item IN LISTS contents)
            get_filename_component(item_name "${item}" NAME)
            if(NOT item_name STREQUAL "_deps")
                list(APPEND remove_paths "${item}")
            endif()
        endforeach()
    else()
        list(APPEND remove_paths "${entry}")
    endif()
endforeach()

# Check every resolved deletion target before removing anything.
foreach(path IN LISTS remove_paths)
    file(REAL_PATH "${path}" resolved_path)
    cmake_path(IS_PREFIX build_root "${resolved_path}" NORMALIZE inside_build)
    if(IS_SYMLINK "${path}" OR NOT inside_build
            OR resolved_path STREQUAL build_root)
        message(FATAL_ERROR "Refusing to remove unsafe build path: ${path}")
    endif()
endforeach()

foreach(path IN LISTS remove_paths)
    file(REMOVE_RECURSE "${path}")
    if(EXISTS "${path}")
        message(FATAL_ERROR "Could not remove ${path}. Stop the app/debugger and try again.")
    endif()
endforeach()
message(STATUS "Cleaned build outputs and CMake caches; kept downloads/ and each preset's _deps/.")
message(STATUS "Run cmake --preset <preset> before building again.")
