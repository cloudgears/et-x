uniform sampler2D texture_diffuse;
uniform sampler2D texture_normal;
uniform sampler2D texture_occlusion;

uniform float aoPower = 6.0;

etFragmentIn vec2 TexCoord;

void main()
{
	vec4 occlusionSample = etTexture2D(texture_occlusion, TexCoord);
	vec4 diffuseSample = etTexture2D(texture_diffuse, TexCoord);
	float occlusion = pow(1.0 - occlusionSample.w, aoPower);
	
	etFragmentOut = diffuseSample * occlusion + occlusionSample;
}
