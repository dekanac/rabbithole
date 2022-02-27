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
glslc.exe -fshader-stage=compute CS_Example.glsl -o CS_Example.spv


pause