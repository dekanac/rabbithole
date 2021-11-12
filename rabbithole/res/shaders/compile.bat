glslc.exe -fshader-stage=vertex VS_PhongBasicTest.glsl -o VS_PhongBasicTest.spv
glslc.exe -fshader-stage=vertex VS_PassThrough.glsl -o VS_PassThrough.spv
glslc.exe -fshader-stage=vertex VS_Gbuffer.glsl -o VS_Gbuffer.spv
glslc.exe -fshader-stage=fragment FS_PhongBasicTest.glsl -o FS_PhongBasicTest.spv
glslc.exe -fshader-stage=fragment FS_PassThrough.glsl -o FS_PassThrough.spv
glslc.exe -fshader-stage=fragment FS_GBuffer.glsl -o FS_GBuffer.spv

pause