#include <et>
#include <inputdefines>
#include <inputlayout>

Texture2D<float4> baseColorTexture : DECL_TEXTURE(BaseColor);
SamplerState baseColorSampler : DECL_SAMPLER(BaseColor);

cbuffer ObjectVariables : DECL_BUFFER(Object)
{
	row_major float4x4 projectionTransform;
};

struct VSOutput 
{
	float4 position : SV_Position;
	float4 color : COLOR0;
	float2 texCoord0 : TEXCOORD0;
};

VSOutput vertexMain(VSInput vsIn)
{
	VSOutput vsOut;
	vsOut.texCoord0 = vsIn.texCoord0.xy;
	vsOut.color = vsIn.color;
	vsOut.position = mul(vsIn.position, projectionTransform);
	return vsOut;
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	return fsIn.color * baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0);
}
