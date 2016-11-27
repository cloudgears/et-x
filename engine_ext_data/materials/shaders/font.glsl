#include <et>

layout (std140, set = VariablesSetIndex, binding = PassVariablesBufferIndex) uniform PassVariables passVariables;

#include <stagedefine>

struct VSOutput {
	vec4 color;
	vec2 texCoord0;
	vec2 params;
};

#if (ET_VERTEX_SHADER)

#include <inputlayout>

layout (location = 0) out VSOutput vsOut;

void main()
{
//  TODO : use additional offset and alpha

	vsOut.color = color;
	vsOut.texCoord0 = texCoord0.xy;
	vsOut.params.x = texCoord0.z - texCoord0.w;
	vsOut.params.y = texCoord0.z + texCoord0.w;
	gl_Position = passVariables.projection * position;
}

#elif (ET_FRAGMENT_SHADER)

layout (set = TexturesSetIndex, binding = AlbedoTextureBinding) uniform sampler2D albedoTexture;
	
layout (location = 0) in VSOutput fsIn;
layout (location = 0) out vec4 outColor0;

void main()
{
	float alpha = texture(albedoTexture, fsIn.texCoord0.xy).x;

	outColor0 = fsIn.color;
	outColor0.w *= smoothstep(fsIn.params.x, fsIn.params.y, alpha);
}

#else
#	error Unsupported shader
#endif

