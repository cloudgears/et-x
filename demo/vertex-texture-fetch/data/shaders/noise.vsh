etVertexIn vec2 Vertex;

etVertexOut vec2 TexCoord;

void main()
{
	TexCoord = Vertex;
	gl_Position = vec4(Vertex, 0.0, 1.0);
}