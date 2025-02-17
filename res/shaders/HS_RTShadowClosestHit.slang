struct InPayload
{
	[[vk::location(0)]] float3 shadowMask;
};

[[vk::binding(0)]] RaytracingAccelerationStructure topLevelAS;

struct Attribs
{
    float3 attribs;
};

[shader("closesthit")]
void main(inout InPayload inPayload, in Attribs attribs)
{
	inPayload.shadowMask = 0;
}
