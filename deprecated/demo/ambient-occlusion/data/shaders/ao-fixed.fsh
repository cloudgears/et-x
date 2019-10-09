#define NUM_SAMPLES	64

uniform sampler2DArray texture_input;

uniform vec3 randomSamples[NUM_SAMPLES];
uniform float sampleLayer;
uniform float depthDifferenceScale = 5.0;
uniform float aoPower = 1.0;

etFragmentIn vec2 TexCoord;

#include "include/viewspace.fsh"

void main()
{
	vec4 centralSample = texture(texture_input, vec3(TexCoord, sampleLayer));

	vec3 centralNormal = centralSample.xyz;
	vec3 centralPosition = vec3((2.0 * TexCoord - 1.0) * texCoordScales, 1.0) * centralSample.w;

	float occlusion = 0.0;
	for (int i = 0; i < NUM_SAMPLES; ++i)
	{
		vec3 targetDirection = randomSamples[i] * sign(dot(randomSamples[i], centralNormal));
		vec3 targetPosition = centralPosition + targetDirection;
		vec2 projectedTexCoord = (targetPosition.xy / texCoordScales) / targetPosition.z;

		float sampledDepth = texture(texture_input, vec3(0.5 + 0.5 * projectedTexCoord, sampleLayer)).w;
		float depthDifference = 5.0 * (sampledDepth - targetPosition.z);

		occlusion += step(0.0, depthDifference) * dot(targetDirection, centralNormal) / (1.0 + depthDifference * depthDifference);
	} 

	etFragmentOut = vec4(pow(1.0 - occlusion / float(NUM_SAMPLES), aoPower));
}
