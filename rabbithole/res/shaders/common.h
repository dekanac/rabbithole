
#define lightCount 4

#define LAYOUT_OUT_VEC2(x) layout(location = x) out vec2
#define LAYOUT_OUT_VEC3(x) layout(location = x) out vec3
#define LAYOUT_IN_VEC2(x) layout(location = x) in vec2
#define LAYOUT_IN_VEC3(x) layout(location = x) in vec3
#define LAYOUT_IN_VEC4(x) layout(location = x) in vec4

#define EPSILON 0.0001

const int LightType_Directional = 0;
const int LightType_Point = 1;
const int LightType_Spot = 2;

struct Light
{
	vec3 position;
	float radius;
	vec3 color;
	float intensity;
	uint type;
    float size;
    float outerConeCos;
    float innerConeCos;
};

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
	mat4 view;
	mat4 proj;
	vec3 cameraPosition;
	vec3 debugOption;
	mat4 viewProjInverse;
	mat4 viewProjMatrix;
	mat4 prevViewProjMatrix;
	mat4 viewInverse;
	mat4 projInverse;
	vec4 frustrumInfo; //x = width, y = height, z = nearPlane, w = farPlane 
	vec4 eyeXAxis;
	vec4 eyeYAxis;
	vec4 eyeZAxis;
	mat4 projJittered;
    vec4 currentFrameInfo; //x = current frame index
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

float ScalarTriple(vec3 a, vec3 b, vec3 c)
{
	return dot(cross(a, b), c);
}

struct MaterialInfo
{
    float perceptualRoughness; // roughness value, as authored by the model creator (input to shader)
    vec3 reflectance0; // full reflectance color (normal incidence angle)

    float alphaRoughness; // roughness mapped to a more linear change in the roughness (proposed by [2])
    vec3 diffuseColor; // color contribution from diffuse lighting

    vec3 reflectance90; // reflectance color at grazing angle
    vec3 specularColor; // color contribution from specular lighting
};

struct SceneInfo
{
	vec3 worldPos;
	vec3 normal;
	vec2 uv;
	vec3 cameraPosition;
};

struct AngularInfo
{
    float NdotL;                  // cos angle between normal and light direction
    float NdotV;                  // cos angle between normal and view direction
    float NdotH;                  // cos angle between normal and half vector
    float LdotH;                  // cos angle between light direction and half vector

    float VdotH;                  // cos angle between view direction and half vector

    vec3 padding;
};

AngularInfo GetAngularInfo(vec3 pointToLight, vec3 normal, vec3 view)
{
    // Standard one-letter names
    vec3 n = normalize(normal);           // Outward direction of surface point
    vec3 v = normalize(view);             // Direction from surface point to view
    vec3 l = normalize(pointToLight);     // Direction from surface point to light
    vec3 h = normalize(l + v);            // Direction of the vector between l and v

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
        vec3(0, 0, 0)
    };

    return angularInfo;
}

vec4 permute(vec4 x) { return mod(((x * 34.0) + 1.0) * x, 289.0); }
vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }
vec3 fade(vec3 t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }

float cnoise(vec3 P)
{
    vec3 Pi0 = floor(P); // Integer part for indexing
    vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
    Pi0 = mod(Pi0, 289.0);
    Pi1 = mod(Pi1, 289.0);
    vec3 Pf0 = fract(P); // Fractional part for interpolation
    vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
    vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
    vec4 iy = vec4(Pi0.yy, Pi1.yy);
    vec4 iz0 = Pi0.zzzz;
    vec4 iz1 = Pi1.zzzz;

    vec4 ixy = permute(permute(ix) + iy);
    vec4 ixy0 = permute(ixy + iz0);
    vec4 ixy1 = permute(ixy + iz1);

    vec4 gx0 = ixy0 / 7.0;
    vec4 gy0 = fract(floor(gx0) / 7.0) - 0.5;
    gx0 = fract(gx0);
    vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
    vec4 sz0 = step(gz0, vec4(0.0));
    gx0 -= sz0 * (step(0.0, gx0) - 0.5);
    gy0 -= sz0 * (step(0.0, gy0) - 0.5);

    vec4 gx1 = ixy1 / 7.0;
    vec4 gy1 = fract(floor(gx1) / 7.0) - 0.5;
    gx1 = fract(gx1);
    vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
    vec4 sz1 = step(gz1, vec4(0.0));
    gx1 -= sz1 * (step(0.0, gx1) - 0.5);
    gy1 -= sz1 * (step(0.0, gy1) - 0.5);

    vec3 g000 = vec3(gx0.x, gy0.x, gz0.x);
    vec3 g100 = vec3(gx0.y, gy0.y, gz0.y);
    vec3 g010 = vec3(gx0.z, gy0.z, gz0.z);
    vec3 g110 = vec3(gx0.w, gy0.w, gz0.w);
    vec3 g001 = vec3(gx1.x, gy1.x, gz1.x);
    vec3 g101 = vec3(gx1.y, gy1.y, gz1.y);
    vec3 g011 = vec3(gx1.z, gy1.z, gz1.z);
    vec3 g111 = vec3(gx1.w, gy1.w, gz1.w);

    vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
    g000 *= norm0.x;
    g010 *= norm0.y;
    g100 *= norm0.z;
    g110 *= norm0.w;
    vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
    g001 *= norm1.x;
    g011 *= norm1.y;
    g101 *= norm1.z;
    g111 *= norm1.w;

    float n000 = dot(g000, Pf0);
    float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
    float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
    float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
    float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
    float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
    float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
    float n111 = dot(g111, Pf1);

    vec3 fade_xyz = fade(Pf0);
    vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
    vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
    float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x);
    return 2.2 * n_xyz;
}

float DistanceSquared(vec3 a, vec3 b)
{
	vec3 v = b - a;
	return dot(v, v);
}