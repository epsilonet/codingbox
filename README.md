# Codingbox

Codingbox is a focused desktop IDE for building and running projects locally. It is built with C++ and Qt Widgets and has a high-contrast blue interface designed around keyboard-driven coding.

![License: BSD-2-Clause](https://img.shields.io/badge/license-BSD--2--Clause-1677b7.svg)
![C++17](https://img.shields.io/badge/C%2B%2B-17-1677b7.svg)
![Qt Widgets](https://img.shields.io/badge/Qt-Widgets-1677b7.svg)

## Features

- Source editor with line numbers, current-line highlighting, syntax colors, tabs, and unsaved-file indicators
- Local workspace explorer with native folder and file dialogs
- Save and Save As support
- Integrated terminal for running local shell commands
- Run support for saved C, C++, Python, JavaScript, and shell files when their toolchains are installed
- Command palette, keyboard shortcuts, custom window controls, and resizable editor and terminal panes
- Flat deep-blue interface with square light-blue borders and a coding-font-first layout

## Download

Prebuilt packages are available from the repository’s [Releases](../../releases) page when a version is published. Choose the package for your operating system and follow the included launch instructions.

## Build from source

Codingbox requires a C++ compiler, CMake, and Qt Widgets. Qt 6 is recommended; Qt 5.15 or newer is also supported.

### Linux

**Debian, Ubuntu, Linux Mint, and related distributions**

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev

git clone https://github.com/epsilonet/codingbox.git
cd codingbox
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/Codingbox
```

**Fedora**

```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel
git clone https://github.com/epsilonet/codingbox.git
cd codingbox
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/Codingbox
```

**Arch Linux**

```bash
sudo pacman -S --needed base-devel cmake qt6-base
git clone https://github.com/epsilonet/codingbox.git
cd codingbox
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/Codingbox
```

### macOS

Install the Xcode command-line tools, CMake, and Qt:

```bash
xcode-select --install
brew install cmake qt

git clone https://github.com/epsilonet/codingbox.git
cd codingbox
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build --parallel
open build/Codingbox.app
```

If Qt was installed with the official Qt installer, replace `CMAKE_PREFIX_PATH` with your Qt installation location, such as `~/Qt/6.8.0/macos`.

### Windows

Install **Visual Studio 2022** with the **Desktop development with C++** workload, [CMake](https://cmake.org/download/), and Qt 6 from the [Qt Online Installer](https://www.qt.io/download-open-source).

Open **x64 Native Tools Command Prompt for VS 2022** and run:

```bat
git clone https://github.com/epsilonet/codingbox.git
cd codingbox
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64"
cmake --build build --config Release --parallel
build\Release\Codingbox.exe
```

Set `CMAKE_PREFIX_PATH` to the Qt kit on your computer. To distribute a local Windows build, run Qt’s `windeployqt` tool against `Codingbox.exe` after compiling.

## Install after building

CMake provides a standard installation target:

```bash
cmake --install build --prefix "$HOME/.local"
```

For multi-configuration generators such as Visual Studio, include `--config Release`.

## License

Codingbox is available under the [BSD 2-Clause License](LICENSE).
