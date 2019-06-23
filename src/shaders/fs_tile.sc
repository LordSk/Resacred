$input v_texcoord0, v_texcoord1, v_alpha_masked

/*
 * Copyright 2014 Dario Manesku. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include <bgfx_shader.sh>

SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_alphaMask, 1);

void main()
{
	vec4 diff = texture2D(s_diffuse, v_texcoord0);
	vec4 mask = texture2D(s_alphaMask, v_texcoord1);
	vec4 color = diff;
	color.a = (1.0-v_alpha_masked) * diff.a + (v_alpha_masked * mask.a);
	gl_FragColor = color;
}