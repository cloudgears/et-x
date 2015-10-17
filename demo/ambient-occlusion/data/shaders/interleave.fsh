uniform int interleave_x;
uniform int interleave_y;

uniform sampler2DArray texture_interleaved;
uniform sampler2DRect texture_original;

etFragmentIn vec2 TexCoord;

#define SHOW_SLICES	0

void main()
{
#if (SHOW_SLICES)

	vec2 interleave = vec2(float(interleave_x), float(interleave_y));
	vec2 scaledTexCoord = interleave * TexCoord;
	int lIndex = int(dot(scaledTexCoord, vec2(1.0, interleave.x)));
	etFragmentOut = texture(texture_interleaved, vec3(mod(scaledTexCoord, 1.0), float(lIndex)));

#else

	ivec2 originalTexCoord = ivec2(gl_FragCoord.xy);
	int col = originalTexCoord.x % interleave_x;
	int row = originalTexCoord.y % interleave_y;
	float layerIndex = float(col + interleave_x * row);

	vec4 interleavedValue = texture(texture_interleaved, vec3(TexCoord, layerIndex));
	vec4 originalValue = texture(texture_original, gl_FragCoord.xy);

	etFragmentOut = interleavedValue;

#endif
}
