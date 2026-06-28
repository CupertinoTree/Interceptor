# Interceptor - Satellite Optical Tracker

A Qt-based application for capturing and displaying live video from ASI cameras with OpenCV integration.

## WARNINGS:

### 1/ Experimental project, use at your own risks

It is intended to be used only by me and with my personal rig (Wave 100i / ZWO ASI planetary cams / macOS) - but it could theoretically work with any ALT/AZ Skywatcher mount.

### 2/ Copyright :

This software is under a GPL License. It does however use code from other open source projects like sgp4 or INDI 3rd party EQMOD. All rights are reserved to the original creators of these code files.

## Prerequisites

### Install Dependencies (macOS with Homebrew)

```bash
# Install Qt6
brew install qt

# Install OpenCV
brew install opencv

# Install build tools
brew install cmake clang-format
```

### Install ASI Camera SDK

The ASI Camera SDK headers and libraries need to be installed. Adjust the paths in `CMakeLists.txt` if necessary:

```cmake
set(ASI_CAMERA_SDK_INCLUDE "/usr/local/include" CACHE PATH "Path to ASI Camera SDK includes")
set(ASI_CAMERA_SDK_LIB "/usr/local/lib" CACHE PATH "Path to ASI Camera SDK libraries")
```

If you have the SDK in a different location, you can either:
1. Create symlinks in `/usr/local/include` and `/usr/local/lib`, or
2. Modify the `CMakeLists.txt` paths to point to your SDK installation

## Building

### Option 1: Using the build script (Recommended)
```bash
chmod +x build.sh
./build.sh
```

### Option 2: Manual CMake build
```bash
mkdir -p build
cd build
cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

## Running

```bash
./build/bin/Interceptor
```

## Features

- **Live Camera Feed**: Display real-time video from connected ASI cameras
- **Camera Selection**: Switch between multiple connected cameras
- **Exposure Control**: Adjust exposure in microseconds or milliseconds
- **Gain Control**: Adjust camera gain
- **FPS Monitor**: Real-time frames per second display
- **Camera Detection**: Automatic detection of connected/disconnected cameras

## Troubleshooting

### "ASICamera2.h not found"
- Verify the ASI Camera SDK is installed
- Check that `ASI_CAMERA_SDK_INCLUDE` path in `CMakeLists.txt` is correct
- Install the SDK if missing

### "Qt not found"
- Verify Qt6 installation: `brew list qt`
- If using Homebrew, Qt should be in `/opt/homebrew/opt/qt/`

### "OpenCV not found"
- Verify OpenCV installation: `brew list opencv`
- If using Homebrew, OpenCV should be in `/opt/homebrew/opt/opencv/`

### Compilation errors with clang++
- Ensure clang++ is available: `which clang++`
- The build script uses clang++ explicitly; if not in PATH, install via Xcode Command Line Tools

## Project Files

- `Interceptor.cpp`: Main application code with UI and camera handling
- `CMakeLists.txt`: Build configuration for CMake
- `build.sh`: Automated build script
- `.gitignore`: Git ignore rules
- `README.md`: This file
