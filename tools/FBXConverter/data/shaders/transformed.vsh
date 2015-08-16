uniform mat4 mModelViewProjection;
uniform vec3 vCamera;
uniform vec3 vPrimaryLight;
uniform mat4 mTransform;

etVertexIn vec3 Vertex;
etVertexIn vec3 Normal;
etVertexIn vec2 TexCoord0;
etVertexIn vec4 Color;

etVertexOut vec2 TexCoord;
etVertexOut vec3 vViewWS;
etVertexOut vec3 vLightWS;
etVertexOut vec3 vNormalWS;
etVertexOut vec4 vColor;

void main()
{
	vec4 aVertex = vec4(Vertex, 1.0);
	vec4 transformedVertex = mTransform * aVertex;
	
	vNormalWS = normalize(mat3(mTransform) * Normal);
	vLightWS = vCamera - transformedVertex.xyz;
	vViewWS = vCamera - transformedVertex.xyz;
	vColor = vColor;
	
	TexCoord  = TexCoord0;
	gl_Position = mModelViewProjection * transformedVertex;
}