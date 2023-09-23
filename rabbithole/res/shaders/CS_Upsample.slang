[[vk::binding(0)]] Sampler2D inputImage;
[[vk::binding(1)]] RWTexture2D<float4> outputImage;

struct PushConstants
{
    uint isLastPass;
};

[[vk::push_constant]] PushConstants Push;

float4 UpsampleTent(Sampler2D tex, float2 uv, float2 texelSize, float4 sampleScale)
{
    float4 d = texelSize.xyxy * float4(1.0, 1.0, -1.0, 0.0) * sampleScale;

    float4 s;
    s  = tex.Sample(uv - d.xy);
    s += tex.Sample(uv - d.wy) * 2.0;
    s += tex.Sample(uv - d.zy);
    s += tex.Sample(uv + d.zw) * 2.0;
    s += tex.Sample(uv       ) * 4.0;
    s += tex.Sample(uv + d.xw) * 2.0;
    s += tex.Sample(uv + d.zy);
    s += tex.Sample(uv + d.wy) * 2.0;
    s += tex.Sample(uv + d.xy);

    return s * (1.0 / 16.0);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
    uint dimX;
    uint dimY;

    inputImage.GetDimensions(dimX, dimY);
    float2 texelSize = 1.f / float2(dimX, dimY);
    float2 uv = (threadId.xy/2 + 0.5f) * texelSize;

    float4 sampleScale = float4(1.f, 1.f, 1.f, 1.f);

    float4 bloom = UpsampleTent(inputImage, uv, texelSize, sampleScale);
    
    float4 currentMip = float4(0.f, 0.f, 0.f, 0.f);

    if (!bool(Push.isLastPass))
        currentMip = outputImage[threadId.xy];
    
    outputImage[threadId.xy] = bloom + currentMip;
}