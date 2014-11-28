uniform sampler2D inputTexture;
uniform float timeValue;

etFragmentIn vec2 TexCoord;

const vec2 sideDir = vec2(1.0 / 1024.0, 0.0);
const vec2 frontDir = vec2(0.0, 1.0 / 1024.0);
const float normalsHeightScale = 0.175;

float sampleHeight(in vec2 tc)
{
	return etTexture2D(inputTexture, tc).x;
}

vec3 calculateNormal(in vec2 tc1, in float c, in float scale)
{
	float px = sampleHeight(tc1 - sideDir);
	float nx = sampleHeight(tc1 + sideDir);
	float py = sampleHeight(tc1 - frontDir);
	float ny = sampleHeight(tc1 + frontDir);
	
	vec3 n1 = cross
	(
		vec3(frontDir.x, scale * (c - py), frontDir.y),
		vec3(sideDir.x, scale * (px - c), sideDir.y)
	 );
	
	vec3 n2 = cross
	(
		vec3(frontDir.x, scale * (ny - c), frontDir.y),
		vec3(sideDir.x, scale * (c - nx), sideDir.y)
	 );
	
	return normalize(n1 + n2);
}

void main()
{
	float c = sampleHeight(TexCoord);
	etFragmentOut = vec4(calculateNormal(TexCoord, c, normalsHeightScale), c);
}
