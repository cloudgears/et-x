#include <et>
#include <inputlayout>

struct VSOutput {
	float4 position : SV_Position;
	float4 color;
	float2 texCoord0;
	float2 params;
};

VSOutput vertexMain(VSInput vsIn)
{
//  TODO : use additional offset and alpha
	VSOutput vsOut;                               
	vsOut.color = vsIn.color;
	vsOut.texCoord0 = vsIn.texCoord0.xy;
	vsOut.params.x = vsIn.texCoord0.z - vsIn.texCoord0.w;
	vsOut.params.y = vsIn.texCoord0.z + vsIn.texCoord0.w;
	vsOut.position = mul(vsIn.position, passVariables.projection);
	return vsOut;
}

// layout (set = TexturesSetIndex, binding = AlbedoTextureBinding) uniform sampler2D albedoTexture;

struct FSOutput
{
	float4 color0 : SV_Target0;
};
	
FSOutput fragmentMain(VSOutput fsIn)
{
	FSOutput fsOut;
	float alpha = 1.0; // texture(albedoTexture, fsIn.texCoord0.xy).x;
	fsOut.color0 = fsIn.color;
	fsOut.color0.w *= smoothstep(fsIn.params.x, fsIn.params.y, alpha);
	return fsOut;
}
