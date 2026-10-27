# 3D Shape Grammars

CS532 final project build instructions.

## Build requirements

- CMake **3.24 or newer**, available as `cmake` in your terminal.
- A C++17 compiler and your platform's OpenGL/GLU development libraries.
- Internet access for the first configure to download FLTK. Later builds reuse
  the source and libraries in `build/`.
- IDE is optional. Both VS Code and Visual Studio can use the same CMake
  project.

FLTK **1.4.4** is downloaded and built automatically from an exact source commit,
with an archive SHA-256 check.

## Windows: build and run

1. Install Visual Studio 2022 or Build Tools 2022 with **Desktop development with
   C++**, the **MSVC v143 x64/x86 tools**, and a **Windows SDK**. If using Visual
   Studio's bundled CMake, also select **C++ CMake tools for Windows**.
2. Install [CMake](https://cmake.org/download/) 3.24+ and add it to `PATH`, or use
   the CMake supplied by a Visual Studio developer terminal. Restart your terminal
   or VS Code after changing `PATH`.
3. Open a terminal in this repository's root: the directory containing this README
   and `CMakeLists.txt`, not the parent directory or `src/`.
4. Configure, build, and run:

   ```powershell
   cmake --preset windows
   cmake --build --preset windows-debug --parallel
   .\build\windows\bin\Debug\shape_grammars.exe
   ```

Or run all three steps with one command from the repository root:

```powershell
.\run
```

`run.cmd` configures the Windows preset, builds Debug, then launches the app.
It stops if configuring or building fails. CMake must be available on `PATH`;
no Make installation or PowerShell execution-policy changes are required.
After a deep clean, the same command downloads and builds FLTK again.

For Release, use the same configured build directory:

```powershell
cmake --build --preset windows-release --parallel
.\build\windows\bin\Release\shape_grammars.exe
```

The first build compiles FLTK and takes longer than subsequent builds. The
application links FLTK statically; no manually copied FLTK DLL is needed. Windows
and the matching Microsoft C++ runtime must still be present. Debug builds are
intended for machines with the C++ development tools installed.

## VS Code

1. Install the platform prerequisites above/below, then open this repository
   with **File > Open Folder**.
2. Install the recommended **C/C++** and **CMake Tools** extensions when prompted, or download from **View > Extensions**.
3. Open the Command Palette with **Ctrl+Shift+P** (macOS: **Cmd+Shift+P**).
4. Run **CMake: Select Configure Preset** and choose **Windows x64 (Visual Studio
   2022)** (`windows`), **macOS Debug** (`macos-debug`), or **Linux Debug**
   (`linux-debug`), as appropriate. Only configure presets for your platform are
   enabled.
5. Run **CMake: Configure**, then **CMake: Select Build Preset** and choose the
   matching Debug build preset.
6. Run **CMake: Build**. Select `shape_grammars` as the launch/debug target if
   prompted, then use **CMake: Run Without Debugging** or **CMake: Debug**.

## Visual Studio 2022

Use **File > Open > Folder** to open the repository. Select the `windows`
configure preset, choose Debug or Release, and set `shape_grammars.exe` as the
startup target. Build and run using the normal Visual Studio controls. You do
not need to create a separate `.vcxproj` or maintain another list of source files.

## macOS

Install the Xcode Command Line Tools:

```sh
xcode-select --install
```

Install CMake 3.24+ and Ninja (for example, with Homebrew):

```sh
brew install cmake ninja
```

Then, from the repository root:

```sh
cmake --preset macos-debug
cmake --build --preset macos-debug --parallel
./build/macos-debug/bin/shape_grammars
```

Use `macos-release` in place of `macos-debug` for Release. FLTK uses native Cocoa
windows. The demo currently uses legacy OpenGL/GLU, which Apple deprecates;
deprecation warnings are expected.

## Linux (Ubuntu/Debian, X11)

Install the compiler, CMake 3.24+, Ninja, and native development libraries:

```sh
sudo apt update
sudo apt install build-essential cmake ninja-build pkg-config \
  libx11-dev libxext-dev libxft-dev libxrender-dev libxfixes-dev \
  libxcursor-dev libxinerama-dev libfontconfig1-dev \
  libgl1-mesa-dev libglu1-mesa-dev
```

Check `cmake --version`; older distributions may require a newer CMake package.
Then, from the repository root:

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug --parallel
./build/linux-debug/bin/shape_grammars
```

Use `linux-release` for Release. The build selects FLTK's X11 backend, so run
within a graphical desktop session (an XWayland session also works). A headless
terminal or WSL installation without a working GUI/OpenGL environment cannot
display the application.

Windows Debug and Release builds are verified. macOS and Linux
presets are provided but have not been validated on those platforms by us.

## Repository development

When integrating another `.cpp` file, add it explicitly to the `shape_grammars`
target in `CMakeLists.txt`.

The same target can also be built without presets:

```sh
cmake -S . -B build/custom
cmake --build build/custom --config Debug --target shape_grammars --parallel
```

The compiler and build tool must be available for the generator selected by
CMake. The provided presets are the recommended starting point.

## Clean build while keeping dependencies

Stop the running application/debugger and wait for any build to finish. From
the repository root, run:

```sh
cmake -P cmake/Clean.cmake
```

This cleans all build directories under `build/`, removing application binaries,
object files, CMake caches, and generated project files. It preserves each build
directory's `_deps/` (FLTK source, download metadata, and compiled libraries) and
`build/downloads/`. System-installed tools and libraries are unaffected.
Custom build directories outside `build/` are not cleaned.

## Deep clean, including dependencies

Stop the application/debugger and any builds, then run from the repository root:

```sh
cmake -P cmake/DeepClean.cmake
```

This deletes the entire `build/` directory, including all presets' generated
files, application binaries, FLTK source and libraries, and cached downloads.

## Troubleshooting

- **`cmake` is not recognized:** install CMake and restart your terminal/editor,
  or open a Visual Studio developer terminal with CMake available.
- **Visual Studio/compiler not found:** install the Windows C++ workload and
  v143 tools. Installing a VS Code extension alone does not install a compiler.
- **FLTK download fails:** check internet/proxy access to `codeload.github.com`,
  then rerun configure. Do not disable certificate or archive hash verification.
  For an offline build, make the pinned FLTK source available locally and pass
  `-DFETCHCONTENT_SOURCE_DIR_FLTK=/absolute/path/to/fltk-1.4.4` when configuring.
  That local source must match the revision declared in `CMakeLists.txt`.
- **OpenGL/GLU or X11 headers are missing:** install the native SDK/development
  packages listed above. FLTK is fetched automatically; system graphics libraries
  come from your OS/SDK.
- **Changing compilers/generators:** use a different preset/build directory.
  Do not reuse a CMake cache configured for another compiler or platform.

## Build references

- [CMake FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html)
- [FLTK 1.4.4 CMake documentation](https://github.com/fltk/fltk/blob/release-1.4.4/README.CMake.txt)
- [VS Code CMake Tools](https://code.visualstudio.com/docs/cpp/cmake-quickstart)
