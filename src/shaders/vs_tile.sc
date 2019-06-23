$input a_position, a_texcoord0, a_texcoord1
$output v_texcoord0, v_texcoord1, v_alpha_masked

#include <bgfx_shader.sh>

void main()
{
	gl_Position = mul(u_modelViewProj, vec4(a_position.xyz, 1.0));
	v_texcoord0 = a_texcoord0;
	v_texcoord1 = a_texcoord1;
	v_alpha_masked  = float(a_texcoord1.x != -1.0);
}
