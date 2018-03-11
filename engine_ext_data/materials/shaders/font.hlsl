#include <et>
#include <inputdefines>
#include <inputlayout>

Texture2D<float> baseColor : DECLARE_TEXTURE;

cbuffer ObjectVariables : DECL_OBJECTS_BUFFER
{
	row_major float4x4 projectionTransform;
	float4 viewport; // x,y - viewport transform, z - viewport alpha
};

struct VSOutput 
{
	float4 position : SV_Position;
	float4 color : COLOR;
	float2 texCoord0 : TEXCOORD0;
	float2 params : TEXCOORD1;
};

VSOutput vertexMain(VSInput vsIn)
{
//  TODO : use additional offset and alpha
	VSOutput vsOut;                               
	vsOut.color = vsIn.color;
	vsOut.texCoord0 = vsIn.texCoord0.xy;
	vsOut.params.x = vsIn.texCoord0.z - vsIn.texCoord0.w;
	vsOut.params.y = vsIn.texCoord0.z + vsIn.texCoord0.w;
	vsOut.position = mul(vsIn.position, projectionTransform) + float4(viewport.xy, 0.0, 0.0);;
	return vsOut;
}
	
float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	float alpha = baseColor.Sample(LinearClamp, fsIn.texCoord0.xy);

	float4 color = fsIn.color;
	color.w *= viewport.z * smoothstep(fsIn.params.x, fsIn.params.y, alpha);

	return color;
}
