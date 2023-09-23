struct VSOutput
{
    float2 outUV : TEXCOORD0;
    float4 position : SV_Position;
};

VSOutput main(uint vertexIdx : SV_VertexID)
{
    VSOutput output;

    output.outUV = float2((vertexIdx << 1) & 2, vertexIdx & 2);
    output.position = float4(output.outUV * 2.0f - 1.0f, 0.0f, 1.0f);

    return output;
}