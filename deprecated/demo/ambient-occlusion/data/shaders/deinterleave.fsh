uniform sampler2DRect texture_normal;
uniform sampler2DRect texture_depth;
uniform vec2 total_interleave;
uniform ivec2 current_interleave;
uniform ivec2 start_position;

#include "include/viewspace.fsh"

layout (location = 0) out vec4 frames[8];

void main()
{
	vec2 baseTexCoord = total_interleave * floor(gl_FragCoord.xy);

	int k = 0;
	for (int y = 0; y < current_interleave.y; ++y)
	{
		for (int x = 0; x < current_interleave.x; ++x)
		{
			ivec2 offset = ivec2(x, y) + start_position;

			vec3 n0 = 2.0 * texture(texture_normal, baseTexCoord + offset).xyz - 1.0;
			float d0 = restoreViewSpaceDistance(texture(texture_depth, baseTexCoord + offset).x);

			frames[k++] = vec4(n0, d0);
		}
	}
}
