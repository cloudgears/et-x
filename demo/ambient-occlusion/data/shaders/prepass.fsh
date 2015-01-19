uniform sampler2D texture_diffuse;
uniform vec4 diffuseColor;

etFragmentIn vec3 vNormalWS;
etFragmentIn vec2 TexCoord;

#include "include/normals.fsh"

void main()
{
	etFragmentOut = etTexture2D(texture_diffuse, TexCoord);
	etFragmentOut1 = vec4(encodeNormal(vNormalWS), 0.0, 0.0);
}
