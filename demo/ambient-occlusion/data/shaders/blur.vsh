uniform vec2 direction;

etVertexIn vec2 Vertex;

#define NUM_SAMPLES 8

etVertexOut vec2 CenterTexCoord;
etVertexOut vec2 PreviousTexCoords[NUM_SAMPLES];
etVertexOut vec2 NextTexCoords[NUM_SAMPLES];

void main()
{
	CenterTexCoord = 0.5 + 0.5 * Vertex;
	
	NextTexCoords[0] = vec2(CenterTexCoord + direction);
	PreviousTexCoords[0] = vec2(CenterTexCoord - direction);
	
	for (int i = 1; i < NUM_SAMPLES; ++i)
	{
		NextTexCoords[i] = NextTexCoords[i-1] + direction;
		PreviousTexCoords[i] = PreviousTexCoords[i-1] - direction;
	}
	
	gl_Position = vec4(Vertex, 0.0, 1.0);
}
