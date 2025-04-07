# Virtual Camera

A simple 3D camera for rendering wireframes.

## Installation

This repository stores third-party libraries in Git submodules, which means
cloning it requires some special care.

Perform recursive cloning using

```sh
git clone --recurse-submodules https://github.com/michalciechan/VirtualCamera
```

## Development

First generate build configuration using CMake.

```sh
cmake -S <project_root> -B build
```

Building the project is done with the following command.

```sh
cmake --build build --target VirtualCamera --config RelWithDebInfo
```
