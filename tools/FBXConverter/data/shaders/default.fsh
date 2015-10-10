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

vec4 validateColorVector(in vec4 vIn)
{
	if (any(isnan(vIn)))
		return vec4(1.0);
	
	if (any(isinf(vIn)))
		return vec4(1.0);
	
	if (dot(vIn, vIn) == 0.0)
		return vec4(1.0);

	if (any(lessThan(vIn, vec4(0.0))))
		return vec4(1.0);
	
	if (any(greaterThan(vIn, vec4(1.0))))
		return vec4(1.0);
	
	return vIn;
}

void main()
{
	vec3 vView = normalize(vViewWS);
	vec3 vLight = normalize(vLightWS);
	vec3 vNormal = normalize(vNormalWS);
	
	vec4 ambientSample = validateColorVector(etTexture2D(ambientMap, TexCoord));
	vec4 diffuseSample = validateColorVector(etTexture2D(diffuseMap, TexCoord));
	vec4 specularSample = validateColorVector(etTexture2D(specularMap, TexCoord));
	vec4 localColor = validateColorVector(vColor);
		
	float diffuseTerm = 0.5 + 0.5 * max(0.0, dot(vNormal, vLight));
	diffuseTerm *= diffuseTerm;

	float specularTerm = pow(max(0.0, dot(reflect(-vLight, vNormal), vView)), 1.0f + roughness);
	
	etFragmentOut = localColor *
		(diffuseSample * (ambientColor * ambientSample + diffuseColor * diffuseTerm) +
		(specularSample * specularColor) * (diffuseSample.w * specularTerm));
	
	etFragmentOut.w = diffuseSample.w;
}