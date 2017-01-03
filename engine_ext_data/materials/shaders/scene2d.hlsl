#include <et>
#include <inputlayout>

struct VSOutput 
{
	float4 position : SV_Position;
	float4 color;
	float4 texCoord0;
};

VSOutput vertexMain(VSInput vsIn)
{
	VSOutput vsOut;
	vsOut.texCoord0 = vsIn.texCoord0;
	vsOut.color = vsIn.color;
	vsOut.position = mul(vsIn.position, passVariables.projection);
	return vsOut;
}

struct FSOutput 
{
	float4 color0 : SV_Target0;
};

FSOutput fragmentMain(VSOutput fsIn)
{
	FSOutput fsOut;
	fsOut.color0 = /* texture(albedoTexture, fsIn.texCoord0.xy) */ fsIn.color;
	return fsOut;
}

