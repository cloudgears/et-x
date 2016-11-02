#include <et>
#include <inputlayout>

/*
 * Inputs / outputs
 */

struct VSOutput {
	float4 position [[position]];
	float4 texCoord0;
	float4 color;
};

struct FSOutput {
	float4 color0 [[color(0)]];
};

// struct ObjectVariables {
//	float4x4 worldTransform;
// };

/*
 * Vertex shader
 */
vertex VSOutput vertexMain(VSInput in [[stage_in]]
	, constant PassVariables& passVariables [[buffer(PassVariablesBufferIndex)]])
{
	VSOutput out;
	out.position = passVariables.projection * in.position;
	out.texCoord0 = in.texCoord0;
	out.color = in.color;
	return out;
}

/*
 * Fragment shader
 */
fragment FSOutput fragmentMain(VSOutput in [[stage_in]])
{
	FSOutput out;
	out.color0 = in.color;
	return out;
}
