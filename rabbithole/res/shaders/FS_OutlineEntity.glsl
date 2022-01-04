#version 450

layout(push_constant) uniform Push {
    uint xMousePos;
    uint yMousePos;
} push;

layout (location = 0) in vec2 inUv;
layout (binding = 0) uniform usampler2D entityIdSampler;

layout (location = 0) out vec4 outColor;

void main()
{
    
    vec2 size = 1.0f / textureSize(entityIdSampler, 0);
    vec2 offset = vec2(push.xMousePos, push.yMousePos) * size;

    uint selectedId = texture(entityIdSampler, offset).r;
        //check if we are targeting chosen entity
	if (texture(entityIdSampler, inUv).r == selectedId && selectedId != 0) //hardcoded: hope we wont have that much entities
    {
        for (int i = -1; i <= +1; i++)
        {
            for (int j = -1; j <= +1; j++)
            {
                if (i == 0 && j == 0)
                {
                    continue;
                }

                offset = vec2(i, j) * size;

                // and if one of the neighboring pixels does not belong to selected entity (we are on the border)
                if (texture(entityIdSampler, inUv + offset).r != selectedId)
                {
                    outColor = vec4(vec3(1.0f), 1.0f);
                    return;
                }
            }
        }
    }

    discard;
}