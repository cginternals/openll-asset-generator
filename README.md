# openll-asset-generator
Font Asset Generator based on OpenLL Specifications

[![GitHub contributors](https://img.shields.io/github/contributors/hpicgs/openll-asset-generator.svg)](https://GitHub.com/hpicgs/openll-asset-generator/graphs/contributors/)
[![MIT license](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/hpicgs/openll-asset-generator/blob/master/LICENSE)

## Project Health

| Service | System | Compiler | Status |
| :------ | ------ | -------- | -----: |
| [Travis-CI](https://travis-ci.org/hpicgs/openll-asset-generator) | Ubuntu 14.04, macOS | GCC 4.8, Clang 3.9 <br> AppleClang 8.1 | [![Travis Build Status](https://img.shields.io/travis/hpicgs/openll-asset-generator.svg)](https://travis-ci.org/hpicgs/openll-asset-generator)|
| [AppVeyor](https://ci.appveyor.com/project/anne-gropler/openll-asset-generator-5cjbt) | Windows | MSVC 2015<br>MSVC 2017 | [![AppVeyor Build Status](https://img.shields.io/appveyor/ci/anne-gropler/openll-asset-generator-5cjbt.svg)](https://ci.appveyor.com/project/anne-gropler/openll-asset-generator-5cjbt)|


## Installation

### Windows

The core library and its applications can be installed either by downloading an installer, e.g., the latest x64 installer for Microsoft Visual Studio 2017, or downloading and extracting one of the precompiled archives, e.g. runtime, examples, and dev. Alternatively, download the source code and commence building from source. See [latest release](https://github.com/hpicgs/openll-asset-generator/releases/latest).

### Ubuntu

We plan to provide the Font Asset Generator using PPAs. Until then, download the source code and commence building from source.

### OS X

We plan to provide the Font Asset Generator via homebrew package manager. Until then, download the source code and commence building from source.

## Build Instructions

### Prerequisites and Dependencies

Necessary for the core library (*llassetgen*) and the CLI application (*llassetgen-cmd*)
* [CMake](https://cmake.org/) 3.4 or higher for building from source
* [Freetype](https://www.freetype.org/) to load and render fonts
* [libpng](http://www.libpng.org/pub/png/libpng.html) to import and export PNGs images.
* [fontconfig](https://www.freedesktop.org/wiki/Software/fontconfig/) on Unix systems. 

Additionally necessary to build the rendering application (*llassetgen-rendering*):
* [GLM](https://github.com/g-truc/glm) for OpenGL math and data structures
* [glbinding](https://github.com/cginternals/glbinding) as OpenGL API binding
* [globjects](https://github.com/cginternals/globjects) to wrap OpenGL API objects
* [Qt5](http://www.qt.io/developers/) 5.0 or higher for GUI elements

### Compile Instructions

For compilation, a C++11 compliant compiler, e.g., GCC 4.8, Clang 5, MSVC 2015, is required.

First, download the source code [as archive](https://github.com/hpicgs/openll-asset-generator/releases). or via git:
```shell
> git clone https://github.com/hpicgs/openll-asset-generator.git
> cd openll-asset-generator
```

Then, depending on the version of globjects you want to build, choose the appropriate tag or branch, e.g., for the 1.0 release:

```shell
> git fetch --tags
> git checkout v1.0-master
```

The actual compilation can be done using CMake and your favorite compiler and IDE.
