uniform mat4 mModelViewProjectionInverse;

etVertexIn vec2 Vertex;
etVertexOut vec3 vUnprojected;

void main()
{
	vec4 vVertex = vec4(Vertex.x, Vertex.y, 1.0, 1.0);
	vUnprojected = (mModelViewProjectionInverse * vVertex).xyz;
	gl_Position = vVertex;
}
