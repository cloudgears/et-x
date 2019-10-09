uniform samplerCube environmentMap;
uniform sampler2D noiseMap;

etFragmentIn etHighp vec3 vUnprojected;

const int samples = 5;

const float dTheta = 0.3926990817;
const float dPhi = 0.7853981634;

void main()
{
	vec3 direction = normalize(vUnprojected);
	
	vec4 color = etTextureCube(environmentMap, direction);
	
	float t0 = asin(direction.y);
	float p0 = atan(direction.x, direction.z);
	
	float theta = t0;
	float phi = p0;
	vec4 noise = 2.0 * etTexture2D(noiseMap, vec2(theta + direction.z, phi + direction.y)) - 1.0;
	
	for (int i = 0; i < samples; ++i)
	{
		noise = 2.0 * etTexture2D(noiseMap, vec2((theta + direction.z) * noise.z, (phi + direction.y) * noise.x)) - 1.0;
		
		theta = t0 + noise.x * dTheta;
		phi = p0 + noise.y * dPhi;
		
		vec3 newDirection;
		newDirection.x = cos(theta) * cos(phi);
		newDirection.y = sin(theta);
		newDirection.z = cos(theta) * sin(phi);
		
		color += etTextureCube(environmentMap, newDirection);
	}
	
	color /= float(samples);
	
	etFragmentOut = color;
}
