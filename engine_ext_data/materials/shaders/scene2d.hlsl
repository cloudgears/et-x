#include <et>
#include <inputdefines>
#include <inputlayout>

Texture2D<float4> baseColorTexture : CONSTANT_LOCATION(t, BaseColorTextureBinding, TexturesSetIndex);
SamplerState baseColorSampler : CONSTANT_LOCATION(s, BaseColorSamplerBinding, TexturesSetIndex);;

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
	vsOut.position = mul(vsIn.position, projection);
	return vsOut;
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	return fsIn.color * baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0);
}
