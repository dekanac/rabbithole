
#define lightCount 4

struct CFBVHNode
{
	float ba, bb, bc;
	float ta, tb, tc;
	uint idxLeft; //if leaf then this is count (if topmost bit set, then this is leaf)
	uint idxRight; //if leaf then this is startIdx in triangle indices
};

bool isLeaf(CFBVHNode node)
{
	return (node.idxLeft & 0x80000000) > 0;
}

struct UniformBufferObject
{
	float4x4 view;
	float4x4 proj;
	float3 cameraPosition;
	float3 debugOption;
	float4x4 viewProjInverse;
	float4x4 viewProjMatrix;
	float4x4 prevViewProjMatrix;
	float4x4 viewInverse;
	float4x4 projInverse;
	float4 frustrumInfo; //x = width, y = height, z = nearPlane, w = farPlane 
	float4 eyeXAxis;
	float4 eyeYAxis;
	float4 eyeZAxis;
	float4x4 projJittered;
    float4 currentFrameInfo; //x = current frame index, y = timePassedSinceStart, // z = deltaTime
};

struct SSAOParams
{
	float radius;
	float bias;
	float resWidth;
	float resHeight;
	float power;
	uint kernelSize;
	bool ssaoOn;
};

struct VolumetricFogParams
{
	uint isEnabled;
	float fogAmount;
    float depthScale_debug;
    float fogStartDistance;
    float fogDistance;
    float fogDepthExponent;
    uint volumetricTexWidth;
    uint volumetricTexHeight;
    uint volumetricTexDepth;
};


#define EPSILON 0.0001

#define LightType_Directional (0)
#define LightType_Point (1)
#define LightType_Spot (2)

struct Light
{
	float3 position;
	float radius;
	float3 color;
	float intensity;
	uint type;
    float size;
    float outerConeCos;
    float innerConeCos;
};

struct MaterialInfo
{
    float perceptualRoughness; // roughness value, as authored by the model creator (input to shader)
    float3 reflectance0;      // full reflectance color (normal incidence angle)

    float alphaRoughness; // roughness mapped to a more linear change in the roughness (proposed by [2])
    float3 diffuseColor;  // color contribution from diffuse lighting

    float3 reflectance90; // reflectance color at grazing angle
    float3 specularColor;  // color contribution from specular lighting
};

struct SceneInfo
{
    float3 worldPos;
    float3 normal;
    float2 uv;
    float3 cameraPosition;
};

struct AngularInfo
{
    float NdotL;    // cos angle between normal and light direction
    float NdotV;    // cos angle between normal and view direction
    float NdotH;    // cos angle between normal and half vector
    float LdotH;    // cos angle between light direction and half vector
    float VdotH;    // cos angle between view direction and half vector
    float3 padding; // Padding to align to a multiple of 16 bytes (required by HLSL)
};

AngularInfo GetAngularInfo(float3 pointToLight, float3 normal, float3 view)
{
    // Standard one-letter names
    float3 n = normalize(normal);       // Outward direction of surface point
    float3 v = normalize(view);         // Direction from surface point to view
    float3 l = normalize(pointToLight); // Direction from surface point to light
    float3 h = normalize(l + v);        // Direction of the vector between l and v

    float NdotL = clamp(dot(n, l), 0.0, 1.0);
    float NdotV = clamp(dot(n, v), 0.0, 1.0);
    float NdotH = clamp(dot(n, h), 0.0, 1.0);
    float LdotH = clamp(dot(l, h), 0.0, 1.0);
    float VdotH = clamp(dot(v, h), 0.0, 1.0);

    AngularInfo angularInfo = {
        NdotL,
        NdotV,
        NdotH,
        LdotH,
        VdotH,
        float3(0, 0, 0) // Padding, adjust based on your alignment requirements
    };

    return angularInfo;
}

static const uint TEMPORAL_FRAMES = 16;

static const float3 halton_map[TEMPORAL_FRAMES] =
    {
        float3(0.5, 0.33333333, 0.2),
        float3(0.25, 0.66666667, 0.4),
        float3(0.75, 0.11111111, 0.6),
        float3(0.125, 0.44444444, 0.8),
        float3(0.625, 0.77777778, 0.04),
        float3(0.375, 0.22222222, 0.24),
        float3(0.875, 0.55555556, 0.44),
        float3(0.0625, 0.88888889, 0.64),
        float3(0.5625, 0.03703704, 0.84),
        float3(0.3125, 0.37037037, 0.08),
        float3(0.8125, 0.7037037, 0.28),
        float3(0.1875, 0.14814815, 0.48),
        float3(0.6875, 0.48148148, 0.68),
        float3(0.4375, 0.81481481, 0.88),
        float3(0.9375, 0.25925926, 0.12),
        float3(0.03125, 0.59259259, 0.32)
    };