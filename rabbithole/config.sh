#!/bin/bash

# Check if an argument is provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 <Debug|RelWithDebInfo|Release>"
    exit 1
fi

# Get the build type from the argument
BUILD_TYPE=$1

# Validate the build type
if [[ "$BUILD_TYPE" != "Debug" && "$BUILD_TYPE" != "RelWithDebInfo" && "$BUILD_TYPE" != "Release" ]]; then
    echo "Invalid build type. Use one of: Debug, RelWithDebInfo, Release"
    exit 1
fi

# Set up build directory
BUILD_DIR="build"

# Create build directory
mkdir -p "$BUILD_DIR"

# Run CMake with the specified build type and Clang
echo "Configuring the project with CMake..."
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

ctags -R .

echo "Built Ctags!"

cp build/compile_commands.json .

echo "Copied compile commands!"

cmake --build "$BUILD_DIR"

echo "Build completed successfully in $BUILD_TYPE mode!"
