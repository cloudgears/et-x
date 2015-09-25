uniform mat4 mModelViewProjection;
uniform vec3 vCamera;
uniform vec3 vPrimaryLight;
uniform mat4 mTransform;

etVertexIn vec3 Vertex;
etVertexIn vec3 Normal;
etVertexIn vec3 Tangent;
etVertexIn vec2 TexCoord0;
etVertexIn vec4 Color;

etVertexOut vec3 vNormalWS;
etVertexOut vec3 vViewWS;
etVertexOut vec3 vLightWS;
etVertexOut vec4 vColor;
etVertexOut vec2 TexCoord;

void main()
{
	vec4 aVertex = vec4(Vertex, 1.0);
	vec4 transformedVertex = mTransform * aVertex;
	
	vNormalWS = normalize(mat3(mTransform) * Normal);
	vLightWS = vPrimaryLight - transformedVertex.xyz;
	vViewWS = vCamera - transformedVertex.xyz;
	vColor = Color;
	
	TexCoord  = TexCoord0;
	gl_Position = mModelViewProjection * transformedVertex;
}
