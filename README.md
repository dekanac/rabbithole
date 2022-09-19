# rabbithole

## 3D Game Engine written in C++ and Vulkan using GLTF models

![img1](https://user-images.githubusercontent.com/34007000/190272509-1d3cc2e8-7d7e-47fc-9fdf-8f6c001065b6.png)
![ezgif-3-b46c2eb673](https://user-images.githubusercontent.com/34007000/190277866-6037d555-3050-49f2-bdee-602c8e6dc0d6.gif)

## rabbithole editor style using Dear ImGui
![2022-09-15 00_14_19-Rabbithole](https://user-images.githubusercontent.com/34007000/190272679-6116bd91-4fe2-4348-bf81-c5c7a2c0e88b.png)

<b>List of features:</b>
- <b>Texture Debugging</b>
![2022-09-15 00_23_58-Rabbithole](https://user-images.githubusercontent.com/34007000/190273592-4d539153-693b-4ef3-b2c6-327ef32e5111.png)
- <b>Light params</b>
- <b>SSAO params</b>
- <b>Volumetric Fog</b>
- <b>GPU Profiler</b>

## raytraced shadows and denoiser
![2022-09-15 00_18_26-rabbithole (Running) - Microsoft Visual Studio (Administrator)](https://user-images.githubusercontent.com/34007000/190273033-6ff6b7f9-32fc-4a52-b211-afa07b863328.png)
- software raytracing using compute shaders
- support for multiple lights (up to 4 for now)
- fully raytraced shadows with soft shadows
- support for omni and directional lighting (point and area)
- fully integrated AMD FX Shadows Denoiser <url>https://gpuopen.com/fidelityfx-denoiser</url>
- tweaking light size will soften the shadow

![ezgif-3-de28a2ad71](https://user-images.githubusercontent.com/34007000/190278226-c2b80f3d-d302-44e5-96c2-728d95ccb027.gif)

## FSR 2.0 upscaling
- upscaling with multiple modes and resolution scale factors
- https://gpuopen.com/fidelityfx-superresolution-2

## Volumetric Fog
![ezgif-3-4ab3a224b1](https://user-images.githubusercontent.com/34007000/190276842-e1d5424d-f693-4150-8ed0-de106a74a9f1.gif)
- volumetric fog with multiple lights support in compute shader with raytraced shadows
- implementation based on Bart Wronski's idea from SIGGRAPH 2014 : <url>https://bartwronski.com/publications/</url>

## other features
- PBR technique for lighting and tweakable basic SSAO for ambient occlusion
- GLTF models
- SPIR-V reflection and VMA for shader reflection and GPU memory allocation
- ImGui <url>https://github.com/ocornut/imgui</url>
