uniform vec2 noiseTextureScale;

etVertexIn vec2 Vertex;
etVertexOut vec2 TexCoord;

void main()
{
	TexCoord = 0.5 + 0.5 * Vertex;
	gl_Position = vec4(Vertex, 0.0, 1.0);
}
