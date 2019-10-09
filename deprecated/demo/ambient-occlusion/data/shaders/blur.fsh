uniform sampler2DRect texture_input;
uniform sampler2DRect texture_normals;
uniform sampler2DRect texture_depth;

uniform float depthDifferenceScale = 5.0;
uniform vec2 direction;
uniform int radius = 8;

#include "include/normals.fsh"
#include "include/viewspace.fsh"

float depthWeight(in float base, in float compared, in int distanceFromCenter)
{
	float falloff = float(radius * radius - distanceFromCenter * distanceFromCenter + 1);
	float depthDifference = 10.0 * depthDifferenceScale * (compared - base);
	return falloff / (1.0 + depthDifference * depthDifference);
}

float normalWeight(in vec3 base, in vec3 compared)
{
	float d = abs(dot(base, compared));
	return 1.0 / (1.0 - 0.5 * d);
}

void main()
{
	vec3 centerNormal = 2.0 * texture(texture_normals, gl_FragCoord.xy).xyz - 1.0;
	float centerDepth = inversesqrt(1.0 - texture(texture_depth, gl_FragCoord.xy).x);

	vec2 sampleCoord = gl_FragCoord.xy - float(radius) * direction;

	float result = 0.0;
	float totalWeight = 0.0;
	for (int i = -radius; i <= radius; ++i)
	{
		float sampledValue = texture(texture_input, sampleCoord).x;
		float depth = inversesqrt(1.0 - texture(texture_depth, sampleCoord).x);
		vec3 normal = 2.0 * texture(texture_normals, sampleCoord).xyz - 1.0;

		float sampleWeight = depthWeight(centerDepth, depth, i) * normalWeight(centerNormal, normal);

		result += sampleWeight * sampledValue;

		totalWeight += sampleWeight;
		sampleCoord += direction;
	}


	etFragmentOut = vec4(result / totalWeight);
}
