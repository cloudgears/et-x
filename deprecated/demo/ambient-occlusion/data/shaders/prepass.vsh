uniform mat4 mModelViewProjection;
uniform mat4 mModelView;
uniform mat4 mTransform;

etVertexIn vec3 Vertex;
etVertexIn vec2 TexCoord0;
etVertexIn vec3 Normal;

etVertexOut vec2 TexCoord;
etVertexOut vec3 vNormalWS;

void main()
{
	TexCoord = TexCoord0;
	vNormalWS = normalize(mat3(mModelView * mTransform) * Normal);
	gl_Position = mModelViewProjection * mTransform * vec4(Vertex, 1.0);
}
