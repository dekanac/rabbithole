glslc.exe -g -fshader-stage=vertex VS_PassThrough.glsl -o VS_PassThrough.spv
glslc.exe -g -fshader-stage=vertex VS_GBuffer.glsl -o VS_GBuffer.spv
glslc.exe -g -fshader-stage=vertex VS_Skybox.glsl -o VS_Skybox.spv
glslc.exe -g -fshader-stage=fragment FS_PBR.glsl -o FS_PBR.spv
glslc.exe -g -fshader-stage=fragment FS_PassThrough.glsl -o FS_PassThrough.spv
glslc.exe -g -fshader-stage=fragment FS_CopyDepth.glsl -o FS_CopyDepth.spv
glslc.exe -g -fshader-stage=fragment FS_GBuffer.glsl -o FS_GBuffer.spv
glslc.exe -g -fshader-stage=fragment FS_OutlineEntity.glsl -o FS_OutlineEntity.spv
glslc.exe -g -fshader-stage=fragment FS_Skybox.glsl -o FS_Skybox.spv
glslc.exe -g -fshader-stage=fragment FS_SSAO.glsl -o FS_SSAO.spv
glslc.exe -g -fshader-stage=compute CS_SSAO.glsl -o CS_SSAO.spv
glslc.exe -g -fshader-stage=fragment FS_SSAOBlur.glsl -o FS_SSAOBlur.spv
glslc.exe -g -fshader-stage=compute CS_RayTracingShadows.glsl -o CS_RayTracingShadows.spv
glslc.exe -g -fshader-stage=vertex VS_SimpleGeometry.glsl -o VS_SimpleGeometry.spv
glslc.exe -g -fshader-stage=fragment FS_SimpleGeometry.glsl -o FS_SimpleGeometry.spv
glslc.exe -g -fshader-stage=compute CS_Volumetric.glsl -o CS_Volumetric.spv
glslc.exe -g -fshader-stage=compute CS_3DNoiseLUT.glsl -o CS_3DNoiseLUT.spv
glslc.exe -g -fshader-stage=compute CS_ComputeScattering.glsl -o CS_ComputeScattering.spv
glslc.exe -g -fshader-stage=fragment FS_ApplyVolumetricFog.glsl -o FS_ApplyVolumetricFog.spv
glslc.exe -g -fshader-stage=fragment FS_Tonemap.glsl -o FS_Tonemap.spv
glslc.exe -g -fshader-stage=fragment FS_TextureDebug.glsl -o FS_TextureDebug.spv
glslc.exe -g -fshader-stage=compute CS_Downsample.glsl -o CS_Downsample.spv
glslc.exe -g -fshader-stage=compute CS_Upsample.glsl -o CS_Upsample.spv
dxc.exe -Zpc -Zi -Qembed_debug -enable-16bit-types -T cs_6_5 -E main -spirv -fspv-target-env=vulkan1.2 CS_PrepareShadowMask.hlsl -Fc IntermediateCS_PrepareShadowMask
dxc.exe -Zpc -Zi -Qembed_debug -enable-16bit-types -T cs_6_5 -E main -spirv -fspv-target-env=vulkan1.2 CS_TileClassification.hlsl -Fc IntermediateCS_TileClassification
dxc.exe -Zpc -Zi -Qembed_debug -enable-16bit-types -T cs_6_5  -E Pass0 -spirv -fspv-target-env=vulkan1.2 CS_FilterSoftShadows.hlsl -Fc IntermediateCS_FilterSoftShadowsP0
dxc.exe -Zpc -Zi -Qembed_debug -enable-16bit-types -T cs_6_5  -E Pass1 -spirv -fspv-target-env=vulkan1.2 CS_FilterSoftShadows.hlsl -Fc IntermediateCS_FilterSoftShadowsP1
dxc.exe -Zpc -Zi -Qembed_debug -enable-16bit-types -T cs_6_5  -E Pass2 -spirv -fspv-target-env=vulkan1.2 CS_FilterSoftShadows.hlsl -Fc IntermediateCS_FilterSoftShadowsP2
spirv-as.exe --target-env vulkan1.2 IntermediateCS_PrepareShadowMask -o CS_PrepareShadowMask.spv
spirv-as.exe --target-env vulkan1.2 IntermediateCS_TileClassification -o CS_TileClassification.spv
spirv-as.exe --target-env vulkan1.2 IntermediateCS_FilterSoftShadowsP0 -o CS_FilterSoftShadowsPass0.spv
spirv-as.exe --target-env vulkan1.2 IntermediateCS_FilterSoftShadowsP1 -o CS_FilterSoftShadowsPass1.spv
spirv-as.exe --target-env vulkan1.2 IntermediateCS_FilterSoftShadowsP2 -o CS_FilterSoftShadowsPass2.spv
DEL	IntermediateCS_PrepareShadowMask
DEL	IntermediateCS_TileClassification 
DEL	IntermediateCS_FilterSoftShadowsP0
DEL	IntermediateCS_FilterSoftShadowsP1
DEL	IntermediateCS_FilterSoftShadowsP2

pause