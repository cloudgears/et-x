uniform sampler2D ambientMap;
uniform sampler2D diffuseMap;
uniform sampler2D specularMap;

uniform vec4 ambientColor;
uniform vec4 diffuseColor;
uniform vec4 specularColor;

uniform float roughness;

etFragmentIn vec3 vNormalWS;
etFragmentIn vec3 vViewWS;
etFragmentIn vec3 vLightWS;
etFragmentIn vec4 vColor;
etFragmentIn vec2 TexCoord;

void main()
{
	vec3 vView = normalize(vViewWS);
	vec3 vLight = normalize(vLightWS);
	vec3 vNormal = normalize(vNormalWS);
	
	vec4 ambientSample = etTexture2D(ambientMap, TexCoord);
	vec4 diffuseSample = etTexture2D(diffuseMap, TexCoord);
	vec4 specularSample = etTexture2D(specularMap, TexCoord);
	vec4 localColor = vColor;
	
	if (dot(ambientSample, ambientSample) == 0.0)
		ambientSample = vec4(1.0);
	
	if (dot(diffuseSample, diffuseSample) == 0.0)
		diffuseSample = vec4(1.0);
	
	if (dot(specularSample, specularSample) == 0.0)
		specularSample = vec4(1.0);

	if (dot(localColor, localColor) == 0.0)
		localColor = vec4(1.0);
	
	float diffuseTerm = 0.5 + 0.5 * max(0.0, dot(vNormal, vLight));
	diffuseTerm *= diffuseTerm;

	float specularTerm = pow(max(0.0, dot(reflect(-vLight, vNormal), vView)), 1.0f + roughness);
	
	etFragmentOut = localColor *
		(diffuseSample * (ambientColor * ambientSample + diffuseColor * diffuseTerm) +
		(specularSample * specularColor) * (diffuseSample.w * specularTerm));
	
	etFragmentOut.w = diffuseSample.w;
}