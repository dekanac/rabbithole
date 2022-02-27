#version 450

layout(rgba8, binding = 0, location = 0) uniform image2D uTex0;
layout(rgba8, binding = 1, location = 1) uniform image2D uTex1;

layout( local_size_x = 16, local_size_y = 16, local_size_z = 1 ) in;
void main()
{

}

