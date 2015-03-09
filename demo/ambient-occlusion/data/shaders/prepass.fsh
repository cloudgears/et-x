uniform sampler2D texture_diffuse;
uniform vec4 diffuseColor;

etFragmentIn vec3 vNormalWS;
etFragmentIn vec2 TexCoord;

#include "include/normals.fsh"

void main()
{
	vec2 originalTextureSize = vec2(textureSize(texture_diffuse, 0));

	etFragmentOut = etTexture2D(texture_diffuse, TexCoord);
	etFragmentOut1 = vec4(0.5 + 0.5 * vNormalWS, 0.0);
}
