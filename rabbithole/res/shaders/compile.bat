glslc.exe -fshader-stage=vertex VS_PassThrough.glsl -o VS_PassThrough.spv
glslc.exe -fshader-stage=vertex VS_Gbuffer.glsl -o VS_Gbuffer.spv
glslc.exe -fshader-stage=vertex VS_Skybox.glsl -o VS_Skybox.spv
glslc.exe -fshader-stage=fragment FS_PhongBasicTest.glsl -o FS_PhongBasicTest.spv
glslc.exe -fshader-stage=fragment FS_PassThrough.glsl -o FS_PassThrough.spv
glslc.exe -fshader-stage=fragment FS_GBuffer.glsl -o FS_GBuffer.spv
glslc.exe -fshader-stage=fragment FS_OutlineEntity.glsl -o FS_OutlineEntity.spv
glslc.exe -fshader-stage=fragment FS_Skybox.glsl -o FS_Skybox.spv

pause