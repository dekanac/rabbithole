#version 450

layout(push_constant) uniform Push {
    uint selectedEntityid;
} push;

layout (location = 0) in vec2 inUv;
layout (binding = 0) uniform usampler2D entityIdSampler;

layout (location = 0) out vec4 outColor;

void main()
{
    if(push.selectedEntityid != 0)
    {
        //check if we are targeting chosen entity
	    if (texture(entityIdSampler, inUv).r == push.selectedEntityid)
        {
            vec2 size = 1.0f / textureSize(entityIdSampler, 0);

            for (int i = -1; i <= +1; i++)
            {
                for (int j = -1; j <= +1; j++)
                {
                    if (i == 0 && j == 0)
                    {
                        continue;
                    }

                    vec2 offset = vec2(i, j) * size;

                    // and if one of the neighboring pixels is white (we are on the border)
                    if (texture(entityIdSampler, inUv + offset).r != push.selectedEntityid)
                    {
                        outColor = vec4(vec3(1.0f), 1.0f);
                        return;
                    }
                }
            }
        }
    }

    discard;
}