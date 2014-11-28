etVertexIn vec2 Vertex;

etVertexOut vec2 TexCoord;

void main()
{
	TexCoord = 0.5 * Vertex + vec2(0.5);
	gl_Position = vec4(Vertex, 0.0, 1.0);
}