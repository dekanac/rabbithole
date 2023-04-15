// Copyright 2020 Google LLC

struct InPayload
{
	[[vk::location(0)]] float3 shadowMask;
};

RaytracingAccelerationStructure topLevelAS : register(t0);

[shader("closesthit")]
void main(in InPayload inPayload, in float3 attribs)
{
	inPayload.shadowMask = 0;
}
