# Codingbox

**Codingbox** is a real, compiled desktop IDE—not a web app or an Electron shell. It is written in modern **C++17** with the native **Qt Widgets** toolkit. The interface deliberately uses a deep-blue palette, light-blue square borders, and a coding-font-first layout.

![License: BSD-2-Clause](https://img.shields.io/badge/license-BSD--2--Clause-1677b7.svg)
![C++17](https://img.shields.io/badge/C%2B%2B-17-1677b7.svg)
![Qt](https://img.shields.io/badge/Qt-Widgets-1677b7.svg)

## What is included

- Native desktop window, custom title bar, window controls, menus, and file dialogs
- A source editor with line numbers, current-line highlighting, syntax colors, tabs, and unsaved-file indicators
- Workspace explorer that can open a real directory on disk
- Save and Save As support for local files
- Resizable integrated terminal that runs local shell commands, command palette, and run workflow
- Run support for saved C/C++, Python, JavaScript, and shell source files using locally installed toolchains
- Flat square styling: no browser chrome, no rounded-card interface, no embedded web runtime

## Download a compiled build

For a ready-to-run version, open this repository’s **[Releases](../../releases)** tab and download the package for your operating system once a version is published.

> If a package for your OS is not available yet, use the source build instructions below.

### Release automation

The cross-platform release workflow is intentionally kept as [`docs/release-workflow.yml.example`](docs/release-workflow.yml.example) until the repository’s GitHub App has **Workflows: write** permission. Once that permission is approved, move it to `.github/workflows/release.yml`; tagged versions will then build and attach Windows, macOS, and Linux packages to GitHub Releases.

## Build from source

Codingbox needs only a C++ compiler, CMake, and Qt Widgets. Qt 6 is preferred; Qt 5.15+ is supported by the CMake project.

### Linux

On Debian, Ubuntu, Linux Mint, or a related distribution:

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev

git clone https://github.com/epsilonet/codingbox.git
cd codingbox
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/Codingbox
```

On Fedora:

```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel
git clone https://github.com/epsilonet/codingbox.git
cd codingbox
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/Codingbox
```

For Arch Linux:

```bash
sudo pacman -S --needed base-devel cmake qt6-base
git clone https://github.com/epsilonet/codingbox.git
cd codingbox
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/Codingbox
```

### macOS

Install the command-line tools, CMake, and Qt:

```bash
xcode-select --install
brew install cmake qt

git clone https://github.com/epsilonet/codingbox.git
cd codingbox
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build --parallel
open build/Codingbox.app
```

If you installed Qt using the official Qt installer rather than Homebrew, replace `CMAKE_PREFIX_PATH` with your Qt installation, for example `~/Qt/6.8.0/macos`.

### Windows

The easiest path is **Visual Studio 2022** with the **Desktop development with C++** workload, [CMake](https://cmake.org/download/), and Qt 6 installed through the [Qt Online Installer](https://www.qt.io/download-open-source).

Open **x64 Native Tools Command Prompt for VS 2022**, then run:

```bat
git clone https://github.com/epsilonet/codingbox.git
cd codingbox
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64"
cmake --build build --config Release --parallel
build\Release\Codingbox.exe
```

The `CMAKE_PREFIX_PATH` should point to the Qt kit installed on your machine. For a redistributable build, run Qt’s `windeployqt` against `Codingbox.exe` after compiling.

### Install after building

CMake also provides a standard install target:

```bash
cmake --install build --prefix "$HOME/.local"
```

On multi-configuration generators such as Visual Studio, add `--config Release`.

## Development

```bash
cmake -S . -B build
cmake --build build --parallel
```

The project intentionally has no Node.js, Chromium, Electron, browser server, or web dependency. See [`CMakeLists.txt`](CMakeLists.txt) for the complete build definition.

## License

Codingbox is released under the [BSD 2-Clause License](LICENSE).
