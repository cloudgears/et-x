#include <et>

layout (std140, set = VariablesSetIndex, binding = PassVariablesBufferIndex) uniform PassVariables passVariables;

#include <stagedefine>

struct VSOutput {
	vec4 color;
	vec4 texCoord0;
};

#if (ET_VERTEX_SHADER)

#include <inputlayout>

layout (location = 0) out VSOutput vsOut;

void main()
{
	vsOut.texCoord0 = texCoord0;
	vsOut.color = color;
	gl_Position = passVariables.projection * position;
}

#elif (ET_FRAGMENT_SHADER)

layout (set = TexturesSetIndex, binding = AlbedoTextureBinding) uniform sampler2D albedoTexture;
	
layout (location = 0) in VSOutput fsIn;
layout (location = 0) out vec4 outColor0;

void main()
{
	outColor0 = texture(albedoTexture, fsIn.texCoord0.xy) * fsIn.color;
}

#else
#	error Unsupported shader
#endif

