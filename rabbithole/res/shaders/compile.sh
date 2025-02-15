#!/bin/bash

# Function to compile a shader with slangc
compile_shader() {
    local input_file=$1
    local output_file=$2
    local profile=$3

    echo "Compiling $input_file to $output_file with profile $profile..."
    slangc -target spirv -profile "$profile" -entry main -fvk-use-entrypoint-name "$input_file" -o "$output_file"

    if [ $? -eq 0 ]; then
        echo "Successfully compiled $input_file to $output_file."
    else
        echo "Failed to compile $input_file."
        exit 1
    fi
}

# Compile shaders with appropriate profiles
compile_shader "RS_RTVolumetricShadow.slang" "RS_RTVolumetricShadow.spv" "sm_6_3"
compile_shader "RS_RTShadowRaygen.slang" "RS_RTShadowRaygen.spv" "sm_6_3"
compile_shader "HS_RTShadowClosestHit.slang" "HS_RTShadowClosestHit.spv" "sm_6_3"

echo "All shaders compiled successfully."

