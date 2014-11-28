uniform samplerCube environmentTexture;
uniform samplerCube reflectionTexture;
uniform sampler2D normalsTexture;

etFragmentIn vec3 vViewWS;
etFragmentIn vec3 vLightWS;
etFragmentIn vec3 vNormalWS;
etFragmentIn vec2 vTextureCoord0;

#include "cooktorrance.h"
#define ROUGHNESS	0.972

const vec4 waterColor = vec4(76.0 / 255.0, 164.0 / 255.0, 177.0 / 255.0, 1.0);

void main()
{
	float viewLength = length(vViewWS);
	float normalSmooth = 2.0 * (1.0 - exp(-0.005 * viewLength));
	float envMixFactor = clamp(10.0 * (0.5 - exp(-0.001 * viewLength)), 0.0, 1.0);
	
	vec4 normalSample = etTexture2D(normalsTexture, vTextureCoord0);
	vec3 n = normalize(normalSample.xyz + vec3(0.0, normalSmooth, 0.0));
	vec3 l = normalize(vLightWS);
	vec3 v = normalize(vViewWS);
	
	float Ks = CookTorrance(n, l, v, ROUGHNESS);
	
	float NdotV = 0.5 + 0.5 * max(0.0, dot(n, v));
	
	vec4 specular = etTextureCube(reflectionTexture, reflect(-v, n));
	vec4 finalColor = mix(waterColor, specular, NdotV);
	
	vec4 background = etTextureCube(reflectionTexture, -vViewWS);

	etFragmentOut = mix(finalColor, background, envMixFactor);
}
