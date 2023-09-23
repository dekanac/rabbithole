
dxc.exe -E main -T lib_6_3 -spirv -fspv-target-env=vulkan1.2 RS_RTVolumetricShadow.hlsl -Fo RS_RTVolumetricShadow.spv
dxc.exe -E main -T lib_6_3 -spirv -fspv-target-env=vulkan1.2 RS_RTShadowRaygen.hlsl -Fo RS_RTShadowRaygen.spv
dxc.exe -E main -T lib_6_3 -spirv -fspv-target-env=vulkan1.2 HS_RTShadowClosestHit.hlsl -Fo HS_RTShadowClosestHit.spv

pause