glslc.exe -fshader-stage=vertex VS_PassThrough.glsl -o VS_PassThrough.spv
glslc.exe -fshader-stage=vertex VS_Gbuffer.glsl -o VS_Gbuffer.spv
glslc.exe -fshader-stage=vertex VS_Skybox.glsl -o VS_Skybox.spv
glslc.exe -fshader-stage=fragment FS_PBR.glsl -o FS_PBR.spv
glslc.exe -fshader-stage=fragment FS_PassThrough.glsl -o FS_PassThrough.spv
glslc.exe -fshader-stage=fragment FS_GBuffer.glsl -o FS_GBuffer.spv
glslc.exe -fshader-stage=fragment FS_OutlineEntity.glsl -o FS_OutlineEntity.spv
glslc.exe -fshader-stage=fragment FS_Skybox.glsl -o FS_Skybox.spv
glslc.exe -fshader-stage=fragment FS_SSAO.glsl -o FS_SSAO.spv
glslc.exe -fshader-stage=fragment FS_SSAOBlur.glsl -o FS_SSAOBlur.spv
glslc.exe -fshader-stage=compute CS_RayTracingShadows.glsl -o CS_RayTracingShadows.spv
glslc.exe -fshader-stage=vertex VS_SimpleGeometry.glsl -o VS_SimpleGeometry.spv
glslc.exe -fshader-stage=fragment FS_SimpleGeometry.glsl -o FS_SimpleGeometry.spv
glslc.exe -fshader-stage=compute -DSAMPLE_EASU=1 -DSAMPLE_SLOW_FALLBACK=1 CS_FSR.glsl -o CS_FSR_EASU.spv
glslc.exe -fshader-stage=compute -DSAMPLE_RCAS=1 -DSAMPLE_SLOW_FALLBACK=1 CS_FSR.glsl -o CS_FSR_RCAS.spv
glslc.exe -fshader-stage=compute CS_TAA.hlsl -o CS_TAA.spv
glslc.exe -fshader-stage=compute CS_TAASharpener.hlsl -o CS_TAASharpener.spv

pause