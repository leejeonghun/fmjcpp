[![License: CC BY-NC-SA 3.0](https://img.shields.io/badge/License-CC%20BY--NC--SA%203.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc-sa/3.0/)

# Full Metal Jacket C++ Port

![screenshot](https://github.com/user-attachments/assets/77f90448-9214-4a3f-b787-a14575363ac5)

Full Metal Jacket (c) 1995, 1996 Mirinae Software, Inc.

## Overview
* This repository is a C++ port of https://github.com/idgmatrix/FMJ using SDL.
* All code in this repository was generated using LLMs.
* Only free-tier LLM services and models were used (Gemini CLI, https://aistudio.google.com).

## Build

### Linux
```bash
cmake -B build -S .
cmake --build build
```

### Windows (Command Prompt for VS)
```bash
vcpkg x-update-baseline --add-initial-baseline
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## License

This project is distributed under the same license as the original repository.

Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

Copyright(c) 1995, 1996 Mirinae Software, Inc.
