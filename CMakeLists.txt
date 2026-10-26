cmake_minimum_required(VERSION 3.24)

project(ShapeGrammars VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build the application and FLTK with the same MSVC runtime.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

find_package(OpenGL REQUIRED)
if(NOT TARGET OpenGL::GLU)
    message(FATAL_ERROR "The demo requires GLU. Install the OpenGL/GLU development libraries for your platform.")
endif()

include(FetchContent)

# FLTK release-1.4.4, pinned to its source commit and archive checksum.
# No locally installed FLTK or machine-specific library paths are required.
set(FLTK_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_GL ON CACHE BOOL "" FORCE)
set(FLTK_BUILD_FORMS OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_FLUID OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_FLTK_OPTIONS OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_HTML_DOCS OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_PDF_DOCS OFF CACHE BOOL "" FORCE)
set(FLTK_MSVC_RUNTIME_DLL ON CACHE BOOL "" FORCE)
set(FLTK_USE_SYSTEM_LIBJPEG OFF CACHE BOOL "" FORCE)
set(FLTK_USE_SYSTEM_LIBPNG OFF CACHE BOOL "" FORCE)
set(FLTK_USE_SYSTEM_ZLIB OFF CACHE BOOL "" FORCE)

# The X11 backend keeps Linux prerequisites smaller than the Wayland backend.
if(UNIX AND NOT APPLE)
    set(FLTK_BACKEND_X11 ON CACHE BOOL "" FORCE)
    set(FLTK_BACKEND_WAYLAND OFF CACHE BOOL "" FORCE)
    set(FLTK_USE_PANGO OFF CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(fltk
    URL https://codeload.github.com/fltk/fltk/tar.gz/9c70ea95a176933ffd575022d145c284d1fc9ef7
    URL_HASH SHA256=e63c5194ac58a32d3c592da2edb7214f08decb7b961ef057f40bb38317df648e
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    TLS_VERIFY TRUE
)
FetchContent_MakeAvailable(fltk)

if(NOT TARGET fltk::gl)
    message(FATAL_ERROR "FLTK OpenGL support was not built. Check your platform's graphics development libraries.")
endif()

# Deliberately list only the current demo sources. Grammar/texture integration
# will be added explicitly later, rather than compiling every file under src/.
add_executable(shape_grammars
    src/main.cpp
    src/view/SceneView.cpp
    src/scene/Camera.cpp
    src/scene/Mesh.cpp
    src/scene/Primitives.cpp

    src/scene/V3.cpp
    src/scene/M33.cpp
    src/grammar/GrammarEngine.cpp
    src/grammar/Rules.cpp
)

target_include_directories(shape_grammars PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/grammar
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene
)
target_link_libraries(shape_grammars PRIVATE
    fltk::fltk
    fltk::gl
    OpenGL::GL
    OpenGL::GLU
)

# Some existing translation units include GL/gl.h directly. On Windows its
# Windows SDK declarations must be available first; keep this platform setup
# in the build configuration instead of changing the renderer sources.
if(WIN32)
    target_compile_definitions(shape_grammars PRIVATE WIN32 NOMINMAX)
    if(MSVC)
        target_compile_options(shape_grammars PRIVATE /FIwindows.h)
    else()
        target_compile_options(shape_grammars PRIVATE -include windows.h)
    endif()
endif()

set_target_properties(shape_grammars PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(run
    COMMAND $<TARGET_FILE:shape_grammars>
    DEPENDS shape_grammars
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    USES_TERMINAL
    COMMENT "Running 3D Shape Grammars"
)
