uniform mat4 mModelViewProjection;
uniform vec3 vCamera;
uniform vec3 vPrimaryLight;
uniform mat4 mTransform;
uniform mat4 bones[32];

etVertexIn vec3 Vertex;
etVertexIn vec3 Normal;
etVertexIn vec2 TexCoord0;
etVertexIn vec4 Color;
etVertexIn vec4 BlendWeights;
etVertexIn ivec4 BlendIndices;

etVertexOut vec2 TexCoord;
etVertexOut vec3 vViewWS;
etVertexOut vec3 vLightWS;
etVertexOut vec3 vNormalWS;
etVertexOut vec4 vColor;

void main()
{
	vec4 aVertex = vec4(Vertex, 1.0);
	
	vec4 transformedVertex =
	(
		BlendWeights.x * (bones[BlendIndices.x] * aVertex) +
		BlendWeights.y * (bones[BlendIndices.y] * aVertex) +
		BlendWeights.z * (bones[BlendIndices.z] * aVertex) +
		BlendWeights.w * (bones[BlendIndices.w] * aVertex)
	);

	vNormalWS = normalize
	(
		BlendWeights.x * (mat3(bones[BlendIndices.x]) * Normal) +
		BlendWeights.y * (mat3(bones[BlendIndices.y]) * Normal) +
		BlendWeights.z * (mat3(bones[BlendIndices.z]) * Normal) +
		BlendWeights.w * (mat3(bones[BlendIndices.w]) * Normal)
	 );
	
	vLightWS = vCamera - transformedVertex.xyz;
	vViewWS = vCamera - transformedVertex.xyz;
	vColor = vColor;
	
	TexCoord  = TexCoord0;
	gl_Position = mModelViewProjection * transformedVertex;
}