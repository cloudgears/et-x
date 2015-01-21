uniform sampler2D texture_diffuse;
uniform sampler2D texture_normal;
uniform sampler2D texture_depth;
uniform sampler2D texture_noise;

uniform int numSamples = 32;
uniform vec2 sampleScale = vec2(0.001, 1.0);
uniform float depthDifferenceScale = 5.0;

etFragmentIn vec2 TexCoord;
etFragmentIn vec2 NoiseTexCoord;

#include "include/viewspace.fsh"
#include "include/normals.fsh"

vec4 performRaytracingInViewSpace(in vec3 vp, in vec3 vn, in vec4 noise, in float minSampleSize, in float maxSampleSize)
{
	vec3 randomNormal = randomVectorOnHemisphere(vn, noise.xyz);
	vec3 projected = projectViewSpacePosition(vp + randomNormal * (minSampleSize + noise.w * maxSampleSize));
	float sampledDepth = etTexture2D(texture_depth, projected.xy).x;
	
	if (sampledDepth > projected.z)
		return vec4(0.0);
	
	float depthDifference = depthDifferenceScale * (inversesqrt(1.0 - projected.z) - inversesqrt(1.0 - sampledDepth));
	float occlusion = dot(vn, randomNormal) * (1.0 - noise.w) / (1.0 + depthDifference * depthDifference);

	return vec4(etTexture2D(texture_diffuse, projected.xy).xyz * occlusion, occlusion);
}

void main()
{
	float borderScale = 1.0 - dot(TexCoord - 0.5, TexCoord - 0.5);
	
	vec4 noiseSample = etTexture2D(texture_noise, NoiseTexCoord);
	vec3 normalSample = decodeNormal(etTexture2D(texture_normal, TexCoord).xy);
	vec3 viewSpacePosition = restoreViewSpacePosition(TexCoord, etTexture2D(texture_depth, TexCoord).x);
	
	vec4 environment = vec4(0.0);

	for (int i = 0; i < numSamples; ++i)
	{
		environment += performRaytracingInViewSpace(viewSpacePosition, normalSample, noiseSample, sampleScale.x, sampleScale.y);
		noiseSample = etTexture2D(texture_noise, noiseSample.xw * NoiseTexCoord + noiseSample.yz);
	}
	
	etFragmentOut = environment / float(numSamples);
}
