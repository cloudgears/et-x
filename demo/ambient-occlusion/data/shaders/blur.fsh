uniform sampler2D texture_color;
uniform sampler2D texture_normal;
uniform sampler2D texture_depth;

uniform float depthDifferenceScale = 5.0;

#define NUM_SAMPLES 8

#include "include/normals.fsh"
#include "include/viewspace.fsh"

etFragmentIn vec2 CenterTexCoord;
etFragmentIn vec2 NextTexCoords[NUM_SAMPLES];
etFragmentIn vec2 PreviousTexCoords[NUM_SAMPLES];

float depthWeight(in float base, in float compared)
{
	float a0 = restoreViewSpaceDistance(base);
	float a1 = restoreViewSpaceDistance(compared);
	return 1.0 / (1.0 + pow(abs(a0 - a1), depthDifferenceScale));
}

float normalWeight(in vec3 base, in vec3 compared)
{
	return abs(dot(base, compared));
}

void main()
{
	float totalWeight = 1.0;
	vec4 result = etTexture2D(texture_color, CenterTexCoord);
	
	vec3 centerNormalSample = decodeNormal(etTexture2D(texture_normal, CenterTexCoord).xy);
	float centerDepthSample = etTexture2D(texture_depth, CenterTexCoord).x;
	
	for (int i = 0; i < NUM_SAMPLES; ++i)
	{
		vec4 nextColor = etTexture2D(texture_color, NextTexCoords[i]);
		vec4 prevColor = etTexture2D(texture_color, PreviousTexCoords[i]);
		
		float nextDepthWeight = depthWeight(centerDepthSample, etTexture2D(texture_depth, NextTexCoords[i]).x);
		float prevDepthWeight = depthWeight(centerDepthSample, etTexture2D(texture_depth, PreviousTexCoords[i]).x);
		
		float nextNormalWeight = normalWeight(centerNormalSample, decodeNormal(etTexture2D(texture_normal, NextTexCoords[i]).xy));
		float prevNormalWeight = normalWeight(centerNormalSample, decodeNormal(etTexture2D(texture_normal, PreviousTexCoords[i]).xy));
		
		float nextWeight = nextNormalWeight * nextDepthWeight;
		float prevWeight = prevNormalWeight * prevDepthWeight;
		
		result += nextColor * nextWeight + prevColor * prevWeight;
		
		totalWeight += nextWeight + prevWeight;
	}
	
	etFragmentOut = vec4(result / totalWeight);
}
