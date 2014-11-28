uniform sampler2D normalsTexture;

uniform mat4 mModelViewProjection;
uniform mat4 mInverseMVPMatrix;
uniform vec3 vCamera;
uniform vec3 vPrimaryLight;
uniform vec2 time;

etVertexIn vec2 Vertex;

etVertexOut vec3 vVertexWS;
etVertexOut vec3 vViewWS;
etVertexOut vec3 vLightWS;
etVertexOut vec2 vTextureCoord0;

void main()
{
	vec4 start = mInverseMVPMatrix * vec4(Vertex, -1.0, 1.0);
	vec4 dir = mInverseMVPMatrix * vec4(Vertex, 1.0, 1.0) - start;
	vec4 vertex = start - (start.y / dir.y) * dir;
	
	vVertexWS = vertex.xyz / vertex.w;
	vTextureCoord0 = vec2(-0.005, 0.0075) * vVertexWS.xz;
	vVertexWS.y += 10.0 * etTexture2D(normalsTexture, vTextureCoord0).w;
	
	vViewWS = vCamera - vVertexWS;
	vLightWS = vPrimaryLight;

	gl_Position = mModelViewProjection * vec4(vVertexWS, 1.0);
}