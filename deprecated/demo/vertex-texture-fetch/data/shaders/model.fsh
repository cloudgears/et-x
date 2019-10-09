uniform samplerCube environmentMap;
uniform samplerCube reflectionMap;

etFragmentIn vec3 vNormalWS;
etFragmentIn vec3 vCameraWS;
etFragmentIn vec3 vLightWS;

#include <cooktorrance.h>
#define ROUGHNESS	0.3174

void main()
{
	vec3 v = normalize(vCameraWS);
	vec3 l = normalize(vLightWS);
	vec3 n = normalize(vNormalWS);
	
	float Kd = max(0.0, 0.5 + 0.5 * dot(l, n));
	float Ks = CookTorrance(n, l, v, ROUGHNESS);
	
	vec4 diffuse =  etTextureCube(environmentMap, n);
	vec4 specular = etTextureCube(reflectionMap, reflect(-v, n));
	
	etFragmentOut = Kd * diffuse + Ks * specular;
}
