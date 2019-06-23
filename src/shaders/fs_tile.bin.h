static const uint8_t fs_tile_glsl[494] =
{
	0x46, 0x53, 0x48, 0x06, 0xc3, 0xef, 0x07, 0x80, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x09, 0x73, // FSH............s
	0x5f, 0x64, 0x69, 0x66, 0x66, 0x75, 0x73, 0x65, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x0b, 0x73, // _diffuse.......s
	0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x4d, 0x61, 0x73, 0x6b, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, // _alphaMask......
	0xb9, 0x01, 0x00, 0x00, 0x76, 0x61, 0x72, 0x79, 0x69, 0x6e, 0x67, 0x20, 0x68, 0x69, 0x67, 0x68, // ....varying high
	0x70, 0x20, 0x66, 0x6c, 0x6f, 0x61, 0x74, 0x20, 0x76, 0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x5f, // p float v_alpha_
	0x6d, 0x61, 0x73, 0x6b, 0x65, 0x64, 0x3b, 0x0a, 0x76, 0x61, 0x72, 0x79, 0x69, 0x6e, 0x67, 0x20, // masked;.varying 
	0x68, 0x69, 0x67, 0x68, 0x70, 0x20, 0x76, 0x65, 0x63, 0x32, 0x20, 0x76, 0x5f, 0x74, 0x65, 0x78, // highp vec2 v_tex
	0x63, 0x6f, 0x6f, 0x72, 0x64, 0x30, 0x3b, 0x0a, 0x76, 0x61, 0x72, 0x79, 0x69, 0x6e, 0x67, 0x20, // coord0;.varying 
	0x68, 0x69, 0x67, 0x68, 0x70, 0x20, 0x76, 0x65, 0x63, 0x32, 0x20, 0x76, 0x5f, 0x74, 0x65, 0x78, // highp vec2 v_tex
	0x63, 0x6f, 0x6f, 0x72, 0x64, 0x31, 0x3b, 0x0a, 0x75, 0x6e, 0x69, 0x66, 0x6f, 0x72, 0x6d, 0x20, // coord1;.uniform 
	0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x72, 0x32, 0x44, 0x20, 0x73, 0x5f, 0x64, 0x69, 0x66, 0x66, // sampler2D s_diff
	0x75, 0x73, 0x65, 0x3b, 0x0a, 0x75, 0x6e, 0x69, 0x66, 0x6f, 0x72, 0x6d, 0x20, 0x73, 0x61, 0x6d, // use;.uniform sam
	0x70, 0x6c, 0x65, 0x72, 0x32, 0x44, 0x20, 0x73, 0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x4d, 0x61, // pler2D s_alphaMa
	0x73, 0x6b, 0x3b, 0x0a, 0x76, 0x6f, 0x69, 0x64, 0x20, 0x6d, 0x61, 0x69, 0x6e, 0x20, 0x28, 0x29, // sk;.void main ()
	0x0a, 0x7b, 0x0a, 0x20, 0x20, 0x6c, 0x6f, 0x77, 0x70, 0x20, 0x76, 0x65, 0x63, 0x34, 0x20, 0x63, // .{.  lowp vec4 c
	0x6f, 0x6c, 0x6f, 0x72, 0x5f, 0x31, 0x3b, 0x0a, 0x20, 0x20, 0x6c, 0x6f, 0x77, 0x70, 0x20, 0x76, // olor_1;.  lowp v
	0x65, 0x63, 0x34, 0x20, 0x74, 0x6d, 0x70, 0x76, 0x61, 0x72, 0x5f, 0x32, 0x3b, 0x0a, 0x20, 0x20, // ec4 tmpvar_2;.  
	0x74, 0x6d, 0x70, 0x76, 0x61, 0x72, 0x5f, 0x32, 0x20, 0x3d, 0x20, 0x74, 0x65, 0x78, 0x74, 0x75, // tmpvar_2 = textu
	0x72, 0x65, 0x32, 0x44, 0x20, 0x28, 0x73, 0x5f, 0x64, 0x69, 0x66, 0x66, 0x75, 0x73, 0x65, 0x2c, // re2D (s_diffuse,
	0x20, 0x76, 0x5f, 0x74, 0x65, 0x78, 0x63, 0x6f, 0x6f, 0x72, 0x64, 0x30, 0x29, 0x3b, 0x0a, 0x20, //  v_texcoord0);. 
	0x20, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x5f, 0x31, 0x2e, 0x78, 0x79, 0x7a, 0x20, 0x3d, 0x20, 0x74, //  color_1.xyz = t
	0x6d, 0x70, 0x76, 0x61, 0x72, 0x5f, 0x32, 0x2e, 0x78, 0x79, 0x7a, 0x3b, 0x0a, 0x20, 0x20, 0x63, // mpvar_2.xyz;.  c
	0x6f, 0x6c, 0x6f, 0x72, 0x5f, 0x31, 0x2e, 0x77, 0x20, 0x3d, 0x20, 0x28, 0x28, 0x28, 0x31, 0x2e, // olor_1.w = (((1.
	0x30, 0x20, 0x2d, 0x20, 0x76, 0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x5f, 0x6d, 0x61, 0x73, 0x6b, // 0 - v_alpha_mask
	0x65, 0x64, 0x29, 0x20, 0x2a, 0x20, 0x74, 0x6d, 0x70, 0x76, 0x61, 0x72, 0x5f, 0x32, 0x2e, 0x77, // ed) * tmpvar_2.w
	0x29, 0x20, 0x2b, 0x20, 0x28, 0x76, 0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x5f, 0x6d, 0x61, 0x73, // ) + (v_alpha_mas
	0x6b, 0x65, 0x64, 0x20, 0x2a, 0x20, 0x74, 0x65, 0x78, 0x74, 0x75, 0x72, 0x65, 0x32, 0x44, 0x20, // ked * texture2D 
	0x28, 0x73, 0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x4d, 0x61, 0x73, 0x6b, 0x2c, 0x20, 0x76, 0x5f, // (s_alphaMask, v_
	0x74, 0x65, 0x78, 0x63, 0x6f, 0x6f, 0x72, 0x64, 0x31, 0x29, 0x2e, 0x77, 0x29, 0x29, 0x3b, 0x0a, // texcoord1).w));.
	0x20, 0x20, 0x67, 0x6c, 0x5f, 0x46, 0x72, 0x61, 0x67, 0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x20, 0x3d, //   gl_FragColor =
	0x20, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x5f, 0x31, 0x3b, 0x0a, 0x7d, 0x0a, 0x0a, 0x00,             //  color_1;.}...
};

static const uint8_t fs_tile_spv[1316] =
{
	0x46, 0x53, 0x48, 0x06, 0xc3, 0xef, 0x07, 0x80, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x09, 0x73, // FSH............s
	0x5f, 0x64, 0x69, 0x66, 0x66, 0x75, 0x73, 0x65, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x73, // _diffuse.......s
	0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x4d, 0x61, 0x73, 0x6b, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, // _alphaMask......
	0xec, 0x04, 0x00, 0x00, 0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x07, 0x00, 0x08, 0x00, // ......#.........
	0xcf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, // ................
	0x0b, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, // ........GLSL.std
	0x2e, 0x34, 0x35, 0x30, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, // .450............
	0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x09, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, // ................
	0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, // main....h...l...
	0x6f, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00, 0x04, 0x00, 0x00, 0x00, // o...|...........
	0x07, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0xf4, 0x01, 0x00, 0x00, // ................
	0x05, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, // ........main....
	0x05, 0x00, 0x07, 0x00, 0x23, 0x00, 0x00, 0x00, 0x73, 0x5f, 0x64, 0x69, 0x66, 0x66, 0x75, 0x73, // ....#...s_diffus
	0x65, 0x53, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x72, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x07, 0x00, // eSampler........
	0x26, 0x00, 0x00, 0x00, 0x73, 0x5f, 0x64, 0x69, 0x66, 0x66, 0x75, 0x73, 0x65, 0x54, 0x65, 0x78, // &...s_diffuseTex
	0x74, 0x75, 0x72, 0x65, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x07, 0x00, 0x2a, 0x00, 0x00, 0x00, // ture........*...
	0x73, 0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x4d, 0x61, 0x73, 0x6b, 0x53, 0x61, 0x6d, 0x70, 0x6c, // s_alphaMaskSampl
	0x65, 0x72, 0x00, 0x00, 0x05, 0x00, 0x07, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x73, 0x5f, 0x61, 0x6c, // er......,...s_al
	0x70, 0x68, 0x61, 0x4d, 0x61, 0x73, 0x6b, 0x54, 0x65, 0x78, 0x74, 0x75, 0x72, 0x65, 0x00, 0x00, // phaMaskTexture..
	0x05, 0x00, 0x06, 0x00, 0x68, 0x00, 0x00, 0x00, 0x76, 0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x5f, // ....h...v_alpha_
	0x6d, 0x61, 0x73, 0x6b, 0x65, 0x64, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x6c, 0x00, 0x00, 0x00, // masked......l...
	0x76, 0x5f, 0x74, 0x65, 0x78, 0x63, 0x6f, 0x6f, 0x72, 0x64, 0x30, 0x00, 0x05, 0x00, 0x05, 0x00, // v_texcoord0.....
	0x6f, 0x00, 0x00, 0x00, 0x76, 0x5f, 0x74, 0x65, 0x78, 0x63, 0x6f, 0x6f, 0x72, 0x64, 0x31, 0x00, // o...v_texcoord1.
	0x05, 0x00, 0x06, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x62, 0x67, 0x66, 0x78, 0x5f, 0x46, 0x72, 0x61, // ....|...bgfx_Fra
	0x67, 0x44, 0x61, 0x74, 0x61, 0x30, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x23, 0x00, 0x00, 0x00, // gData0..G...#...
	0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x23, 0x00, 0x00, 0x00, // ".......G...#...
	0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x26, 0x00, 0x00, 0x00, // !.......G...&...
	0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x26, 0x00, 0x00, 0x00, // ".......G...&...
	0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x2a, 0x00, 0x00, 0x00, // !.......G...*...
	0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x2a, 0x00, 0x00, 0x00, // ".......G...*...
	0x21, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x2c, 0x00, 0x00, 0x00, // !.......G...,...
	0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x2c, 0x00, 0x00, 0x00, // ".......G...,...
	0x21, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x68, 0x00, 0x00, 0x00, // !.......G...h...
	0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x6c, 0x00, 0x00, 0x00, // ........G...l...
	0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x6f, 0x00, 0x00, 0x00, // ........G...o...
	0x1e, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x7c, 0x00, 0x00, 0x00, // ........G...|...
	0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, // ................
	0x21, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x02, 0x00, // !...............
	0x06, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00, 0x07, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, // ............ ...
	0x19, 0x00, 0x09, 0x00, 0x08, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, // ................
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, // ................
	0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, // ................
	0x02, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, // ................
	0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // .... ...".......
	0x06, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x22, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, // ....;..."...#...
	0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // .... ...%.......
	0x08, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x25, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, // ....;...%...&...
	0x00, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x22, 0x00, 0x00, 0x00, 0x2a, 0x00, 0x00, 0x00, // ....;..."...*...
	0x00, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x25, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, // ....;...%...,...
	0x00, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x03, 0x00, 0x38, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, // ........8.......
	0x2b, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x57, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, // +.......W......?
	0x20, 0x00, 0x04, 0x00, 0x67, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, //  ...g...........
	0x3b, 0x00, 0x04, 0x00, 0x67, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, // ;...g...h.......
	0x20, 0x00, 0x04, 0x00, 0x6b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, //  ...k...........
	0x3b, 0x00, 0x04, 0x00, 0x6b, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, // ;...k...l.......
	0x3b, 0x00, 0x04, 0x00, 0x6b, 0x00, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, // ;...k...o.......
	0x20, 0x00, 0x04, 0x00, 0x7b, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, //  ...{...........
	0x3b, 0x00, 0x04, 0x00, 0x7b, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, // ;...{...|.......
	0x36, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 6...............
	0x03, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, // ............=...
	0x06, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, // ....$...#...=...
	0x08, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, // ....'...&...=...
	0x06, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, // ....+...*...=...
	0x08, 0x00, 0x00, 0x00, 0x2d, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, // ....-...,...=...
	0x07, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, // ....i...h...=...
	0x0b, 0x00, 0x00, 0x00, 0x6d, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, // ....m...l...=...
	0x0b, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x00, 0x56, 0x00, 0x05, 0x00, // ....p...o...V...
	0x38, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, // 8.......'...$...
	0x57, 0x00, 0x05, 0x00, 0x0d, 0x00, 0x00, 0x00, 0xba, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, // W...............
	0x6d, 0x00, 0x00, 0x00, 0x56, 0x00, 0x05, 0x00, 0x38, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, // m...V...8.......
	0x2d, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x57, 0x00, 0x05, 0x00, 0x0d, 0x00, 0x00, 0x00, // -...+...W.......
	0xc2, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x83, 0x00, 0x05, 0x00, // ........p.......
	0x07, 0x00, 0x00, 0x00, 0xa2, 0x00, 0x00, 0x00, 0x57, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, // ........W...i...
	0x51, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00, 0x00, 0xba, 0x00, 0x00, 0x00, // Q...............
	0x03, 0x00, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0xa5, 0x00, 0x00, 0x00, // ................
	0xa2, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, // ........Q.......
	0xa8, 0x00, 0x00, 0x00, 0xc2, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00, // ................
	0x07, 0x00, 0x00, 0x00, 0xa9, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0xa8, 0x00, 0x00, 0x00, // ........i.......
	0x81, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0xaa, 0x00, 0x00, 0x00, 0xa5, 0x00, 0x00, 0x00, // ................
	0xa9, 0x00, 0x00, 0x00, 0x52, 0x00, 0x06, 0x00, 0x0d, 0x00, 0x00, 0x00, 0xce, 0x00, 0x00, 0x00, // ....R...........
	0xaa, 0x00, 0x00, 0x00, 0xba, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, // ............>...
	0x7c, 0x00, 0x00, 0x00, 0xce, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00, // |...........8...
	0x00, 0x00, 0x00, 0x00,                                                                         // ....
};

static const uint8_t fs_tile_dx9[365] =
{
	0x46, 0x53, 0x48, 0x06, 0xc3, 0xef, 0x07, 0x80, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x0b, 0x73, // FSH............s
	0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x4d, 0x61, 0x73, 0x6b, 0x30, 0x01, 0x01, 0x00, 0x01, 0x00, // _alphaMask0.....
	0x09, 0x73, 0x5f, 0x64, 0x69, 0x66, 0x66, 0x75, 0x73, 0x65, 0x30, 0x01, 0x00, 0x00, 0x01, 0x00, // .s_diffuse0.....
	0x38, 0x01, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xfe, 0xff, 0x2c, 0x00, 0x43, 0x54, 0x41, 0x42, // 8.........,.CTAB
	0x1c, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0x02, 0x00, 0x00, 0x00, // ................
	0x1c, 0x00, 0x00, 0x00, 0x00, 0x91, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, // ........|...D...
	0x03, 0x00, 0x01, 0x00, 0x01, 0x00, 0x06, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........P.......
	0x60, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x6c, 0x00, 0x00, 0x00, // `...........l...
	0x00, 0x00, 0x00, 0x00, 0x73, 0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x4d, 0x61, 0x73, 0x6b, 0x00, // ....s_alphaMask.
	0x04, 0x00, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ................
	0x73, 0x5f, 0x64, 0x69, 0x66, 0x66, 0x75, 0x73, 0x65, 0x00, 0xab, 0xab, 0x04, 0x00, 0x0c, 0x00, // s_diffuse.......
	0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x73, 0x5f, 0x33, // ............ps_3
	0x5f, 0x30, 0x00, 0x4d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, // _0.Microsoft (R)
	0x20, 0x48, 0x4c, 0x53, 0x4c, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x20, 0x43, 0x6f, 0x6d, //  HLSL Shader Com
	0x70, 0x69, 0x6c, 0x65, 0x72, 0x20, 0x31, 0x30, 0x2e, 0x31, 0x00, 0xab, 0x1f, 0x00, 0x00, 0x02, // piler 10.1......
	0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x01, 0x90, 0x1f, 0x00, 0x00, 0x02, 0x05, 0x00, 0x00, 0x80, // ................
	0x01, 0x00, 0x03, 0x90, 0x1f, 0x00, 0x00, 0x02, 0x05, 0x00, 0x01, 0x80, 0x02, 0x00, 0x03, 0x90, // ................
	0x1f, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x90, 0x00, 0x08, 0x0f, 0xa0, 0x1f, 0x00, 0x00, 0x02, // ................
	0x00, 0x00, 0x00, 0x90, 0x01, 0x08, 0x0f, 0xa0, 0x42, 0x00, 0x00, 0x03, 0x00, 0x00, 0x0f, 0x80, // ........B.......
	0x02, 0x00, 0xe4, 0x90, 0x01, 0x08, 0xe4, 0xa0, 0x42, 0x00, 0x00, 0x03, 0x01, 0x00, 0x0f, 0x80, // ........B.......
	0x01, 0x00, 0xe4, 0x90, 0x00, 0x08, 0xe4, 0xa0, 0x12, 0x00, 0x00, 0x04, 0x00, 0x08, 0x08, 0x80, // ................
	0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0xff, 0x80, 0x01, 0x00, 0xff, 0x80, 0x01, 0x00, 0x00, 0x02, // ................
	0x00, 0x08, 0x07, 0x80, 0x01, 0x00, 0xe4, 0x80, 0xff, 0xff, 0x00, 0x00, 0x00,                   // .............
};

static const uint8_t fs_tile_dx11[620] =
{
	0x46, 0x53, 0x48, 0x06, 0xc3, 0xef, 0x07, 0x80, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x09, 0x73, // FSH............s
	0x5f, 0x64, 0x69, 0x66, 0x66, 0x75, 0x73, 0x65, 0x30, 0x01, 0x00, 0x00, 0x01, 0x00, 0x0b, 0x73, // _diffuse0......s
	0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x4d, 0x61, 0x73, 0x6b, 0x30, 0x01, 0x01, 0x00, 0x01, 0x00, // _alphaMask0.....
	0x34, 0x02, 0x00, 0x00, 0x44, 0x58, 0x42, 0x43, 0xe3, 0xa5, 0x0b, 0xe9, 0x5b, 0x59, 0x3d, 0x08, // 4...DXBC....[Y=.
	0x35, 0xe0, 0xd9, 0x37, 0x80, 0xa8, 0x60, 0x7f, 0x01, 0x00, 0x00, 0x00, 0x34, 0x02, 0x00, 0x00, // 5..7..`.....4...
	0x03, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0xf4, 0x00, 0x00, 0x00, // ....,...........
	0x49, 0x53, 0x47, 0x4e, 0x8c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, // ISGN............
	0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, // h...............
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........t.......
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, // ................
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, // ................
	0x01, 0x00, 0x00, 0x00, 0x06, 0x06, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, // ................
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, // ................
	0x53, 0x56, 0x5f, 0x50, 0x4f, 0x53, 0x49, 0x54, 0x49, 0x4f, 0x4e, 0x00, 0x42, 0x4c, 0x45, 0x4e, // SV_POSITION.BLEN
	0x44, 0x57, 0x45, 0x49, 0x47, 0x48, 0x54, 0x00, 0x54, 0x45, 0x58, 0x43, 0x4f, 0x4f, 0x52, 0x44, // DWEIGHT.TEXCOORD
	0x00, 0xab, 0xab, 0xab, 0x4f, 0x53, 0x47, 0x4e, 0x2c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, // ....OSGN,.......
	0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // .... ...........
	0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x53, 0x56, 0x5f, 0x54, // ............SV_T
	0x41, 0x52, 0x47, 0x45, 0x54, 0x00, 0xab, 0xab, 0x53, 0x48, 0x44, 0x52, 0x38, 0x01, 0x00, 0x00, // ARGET...SHDR8...
	0x40, 0x00, 0x00, 0x00, 0x4e, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x00, 0x03, 0x00, 0x60, 0x10, 0x00, // @...N...Z....`..
	0x00, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x00, 0x03, 0x00, 0x60, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, // ....Z....`......
	0x58, 0x18, 0x00, 0x04, 0x00, 0x70, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x55, 0x00, 0x00, // X....p......UU..
	0x58, 0x18, 0x00, 0x04, 0x00, 0x70, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x55, 0x55, 0x00, 0x00, // X....p......UU..
	0x62, 0x10, 0x00, 0x03, 0x12, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x62, 0x10, 0x00, 0x03, // b...........b...
	0x62, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x62, 0x10, 0x00, 0x03, 0x32, 0x10, 0x10, 0x00, // b.......b...2...
	0x02, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0xf2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, // ....e.... ......
	0x68, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x09, 0xf2, 0x00, 0x10, 0x00, // h.......E.......
	0x00, 0x00, 0x00, 0x00, 0x46, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x46, 0x7e, 0x10, 0x00, // ....F.......F~..
	0x01, 0x00, 0x00, 0x00, 0x00, 0x60, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x07, // .....`......8...
	0x12, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3a, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, // ........:.......
	0x0a, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x22, 0x00, 0x10, 0x00, // ............"...
	0x00, 0x00, 0x00, 0x00, 0x0a, 0x10, 0x10, 0x80, 0x41, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, // ........A.......
	0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x45, 0x00, 0x00, 0x09, 0xf2, 0x00, 0x10, 0x00, // .@.....?E.......
	0x01, 0x00, 0x00, 0x00, 0x96, 0x15, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x7e, 0x10, 0x00, // ............F~..
	0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x09, // .....`......2...
	0x82, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, // . ..............
	0x3a, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, // :...............
	0x36, 0x00, 0x00, 0x05, 0x72, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x02, 0x10, 0x00, // 6...r ......F...
	0x01, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,                         // ....>.......
};

static const uint8_t fs_tile_mtl[896] =
{
	0x46, 0x53, 0x48, 0x06, 0xc3, 0xef, 0x07, 0x80, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x09, 0x73, // FSH............s
	0x5f, 0x64, 0x69, 0x66, 0x66, 0x75, 0x73, 0x65, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x73, // _diffuse.......s
	0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x4d, 0x61, 0x73, 0x6b, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, // _alphaMask......
	0x48, 0x03, 0x00, 0x00, 0x23, 0x69, 0x6e, 0x63, 0x6c, 0x75, 0x64, 0x65, 0x20, 0x3c, 0x6d, 0x65, // H...#include <me
	0x74, 0x61, 0x6c, 0x5f, 0x73, 0x74, 0x64, 0x6c, 0x69, 0x62, 0x3e, 0x0a, 0x23, 0x69, 0x6e, 0x63, // tal_stdlib>.#inc
	0x6c, 0x75, 0x64, 0x65, 0x20, 0x3c, 0x73, 0x69, 0x6d, 0x64, 0x2f, 0x73, 0x69, 0x6d, 0x64, 0x2e, // lude <simd/simd.
	0x68, 0x3e, 0x0a, 0x0a, 0x75, 0x73, 0x69, 0x6e, 0x67, 0x20, 0x6e, 0x61, 0x6d, 0x65, 0x73, 0x70, // h>..using namesp
	0x61, 0x63, 0x65, 0x20, 0x6d, 0x65, 0x74, 0x61, 0x6c, 0x3b, 0x0a, 0x0a, 0x73, 0x74, 0x72, 0x75, // ace metal;..stru
	0x63, 0x74, 0x20, 0x78, 0x6c, 0x61, 0x74, 0x4d, 0x74, 0x6c, 0x4d, 0x61, 0x69, 0x6e, 0x5f, 0x6f, // ct xlatMtlMain_o
	0x75, 0x74, 0x0a, 0x7b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x66, 0x6c, 0x6f, 0x61, 0x74, 0x34, 0x20, // ut.{.    float4 
	0x62, 0x67, 0x66, 0x78, 0x5f, 0x46, 0x72, 0x61, 0x67, 0x44, 0x61, 0x74, 0x61, 0x30, 0x20, 0x5b, // bgfx_FragData0 [
	0x5b, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x28, 0x30, 0x29, 0x5d, 0x5d, 0x3b, 0x0a, 0x7d, 0x3b, 0x0a, // [color(0)]];.};.
	0x0a, 0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x20, 0x78, 0x6c, 0x61, 0x74, 0x4d, 0x74, 0x6c, 0x4d, // .struct xlatMtlM
	0x61, 0x69, 0x6e, 0x5f, 0x69, 0x6e, 0x0a, 0x7b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x66, 0x6c, 0x6f, // ain_in.{.    flo
	0x61, 0x74, 0x20, 0x76, 0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x5f, 0x6d, 0x61, 0x73, 0x6b, 0x65, // at v_alpha_maske
	0x64, 0x20, 0x5b, 0x5b, 0x75, 0x73, 0x65, 0x72, 0x28, 0x6c, 0x6f, 0x63, 0x6e, 0x30, 0x29, 0x5d, // d [[user(locn0)]
	0x5d, 0x3b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x66, 0x6c, 0x6f, 0x61, 0x74, 0x32, 0x20, 0x76, 0x5f, // ];.    float2 v_
	0x74, 0x65, 0x78, 0x63, 0x6f, 0x6f, 0x72, 0x64, 0x30, 0x20, 0x5b, 0x5b, 0x75, 0x73, 0x65, 0x72, // texcoord0 [[user
	0x28, 0x6c, 0x6f, 0x63, 0x6e, 0x31, 0x29, 0x5d, 0x5d, 0x3b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x66, // (locn1)]];.    f
	0x6c, 0x6f, 0x61, 0x74, 0x32, 0x20, 0x76, 0x5f, 0x74, 0x65, 0x78, 0x63, 0x6f, 0x6f, 0x72, 0x64, // loat2 v_texcoord
	0x31, 0x20, 0x5b, 0x5b, 0x75, 0x73, 0x65, 0x72, 0x28, 0x6c, 0x6f, 0x63, 0x6e, 0x32, 0x29, 0x5d, // 1 [[user(locn2)]
	0x5d, 0x3b, 0x0a, 0x7d, 0x3b, 0x0a, 0x0a, 0x66, 0x72, 0x61, 0x67, 0x6d, 0x65, 0x6e, 0x74, 0x20, // ];.};..fragment 
	0x78, 0x6c, 0x61, 0x74, 0x4d, 0x74, 0x6c, 0x4d, 0x61, 0x69, 0x6e, 0x5f, 0x6f, 0x75, 0x74, 0x20, // xlatMtlMain_out 
	0x78, 0x6c, 0x61, 0x74, 0x4d, 0x74, 0x6c, 0x4d, 0x61, 0x69, 0x6e, 0x28, 0x78, 0x6c, 0x61, 0x74, // xlatMtlMain(xlat
	0x4d, 0x74, 0x6c, 0x4d, 0x61, 0x69, 0x6e, 0x5f, 0x69, 0x6e, 0x20, 0x69, 0x6e, 0x20, 0x5b, 0x5b, // MtlMain_in in [[
	0x73, 0x74, 0x61, 0x67, 0x65, 0x5f, 0x69, 0x6e, 0x5d, 0x5d, 0x2c, 0x20, 0x74, 0x65, 0x78, 0x74, // stage_in]], text
	0x75, 0x72, 0x65, 0x32, 0x64, 0x3c, 0x66, 0x6c, 0x6f, 0x61, 0x74, 0x3e, 0x20, 0x73, 0x5f, 0x64, // ure2d<float> s_d
	0x69, 0x66, 0x66, 0x75, 0x73, 0x65, 0x20, 0x5b, 0x5b, 0x74, 0x65, 0x78, 0x74, 0x75, 0x72, 0x65, // iffuse [[texture
	0x28, 0x30, 0x29, 0x5d, 0x5d, 0x2c, 0x20, 0x74, 0x65, 0x78, 0x74, 0x75, 0x72, 0x65, 0x32, 0x64, // (0)]], texture2d
	0x3c, 0x66, 0x6c, 0x6f, 0x61, 0x74, 0x3e, 0x20, 0x73, 0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x4d, // <float> s_alphaM
	0x61, 0x73, 0x6b, 0x20, 0x5b, 0x5b, 0x74, 0x65, 0x78, 0x74, 0x75, 0x72, 0x65, 0x28, 0x31, 0x29, // ask [[texture(1)
	0x5d, 0x5d, 0x2c, 0x20, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x72, 0x20, 0x73, 0x5f, 0x64, 0x69, // ]], sampler s_di
	0x66, 0x66, 0x75, 0x73, 0x65, 0x53, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x72, 0x20, 0x5b, 0x5b, 0x73, // ffuseSampler [[s
	0x61, 0x6d, 0x70, 0x6c, 0x65, 0x72, 0x28, 0x30, 0x29, 0x5d, 0x5d, 0x2c, 0x20, 0x73, 0x61, 0x6d, // ampler(0)]], sam
	0x70, 0x6c, 0x65, 0x72, 0x20, 0x73, 0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x4d, 0x61, 0x73, 0x6b, // pler s_alphaMask
	0x53, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x72, 0x20, 0x5b, 0x5b, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, // Sampler [[sample
	0x72, 0x28, 0x31, 0x29, 0x5d, 0x5d, 0x29, 0x0a, 0x7b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x78, 0x6c, // r(1)]]).{.    xl
	0x61, 0x74, 0x4d, 0x74, 0x6c, 0x4d, 0x61, 0x69, 0x6e, 0x5f, 0x6f, 0x75, 0x74, 0x20, 0x6f, 0x75, // atMtlMain_out ou
	0x74, 0x20, 0x3d, 0x20, 0x7b, 0x7d, 0x3b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x66, 0x6c, 0x6f, 0x61, // t = {};.    floa
	0x74, 0x34, 0x20, 0x5f, 0x31, 0x38, 0x36, 0x20, 0x3d, 0x20, 0x73, 0x5f, 0x64, 0x69, 0x66, 0x66, // t4 _186 = s_diff
	0x75, 0x73, 0x65, 0x2e, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x28, 0x73, 0x5f, 0x64, 0x69, 0x66, // use.sample(s_dif
	0x66, 0x75, 0x73, 0x65, 0x53, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x72, 0x2c, 0x20, 0x69, 0x6e, 0x2e, // fuseSampler, in.
	0x76, 0x5f, 0x74, 0x65, 0x78, 0x63, 0x6f, 0x6f, 0x72, 0x64, 0x30, 0x29, 0x3b, 0x0a, 0x20, 0x20, // v_texcoord0);.  
	0x20, 0x20, 0x66, 0x6c, 0x6f, 0x61, 0x74, 0x34, 0x20, 0x5f, 0x32, 0x30, 0x36, 0x20, 0x3d, 0x20, //   float4 _206 = 
	0x5f, 0x31, 0x38, 0x36, 0x3b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x5f, 0x32, 0x30, 0x36, 0x2e, 0x77, // _186;.    _206.w
	0x20, 0x3d, 0x20, 0x28, 0x28, 0x31, 0x2e, 0x30, 0x20, 0x2d, 0x20, 0x69, 0x6e, 0x2e, 0x76, 0x5f, //  = ((1.0 - in.v_
	0x61, 0x6c, 0x70, 0x68, 0x61, 0x5f, 0x6d, 0x61, 0x73, 0x6b, 0x65, 0x64, 0x29, 0x20, 0x2a, 0x20, // alpha_masked) * 
	0x5f, 0x31, 0x38, 0x36, 0x2e, 0x77, 0x29, 0x20, 0x2b, 0x20, 0x28, 0x69, 0x6e, 0x2e, 0x76, 0x5f, // _186.w) + (in.v_
	0x61, 0x6c, 0x70, 0x68, 0x61, 0x5f, 0x6d, 0x61, 0x73, 0x6b, 0x65, 0x64, 0x20, 0x2a, 0x20, 0x73, // alpha_masked * s
	0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x4d, 0x61, 0x73, 0x6b, 0x2e, 0x73, 0x61, 0x6d, 0x70, 0x6c, // _alphaMask.sampl
	0x65, 0x28, 0x73, 0x5f, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x4d, 0x61, 0x73, 0x6b, 0x53, 0x61, 0x6d, // e(s_alphaMaskSam
	0x70, 0x6c, 0x65, 0x72, 0x2c, 0x20, 0x69, 0x6e, 0x2e, 0x76, 0x5f, 0x74, 0x65, 0x78, 0x63, 0x6f, // pler, in.v_texco
	0x6f, 0x72, 0x64, 0x31, 0x29, 0x2e, 0x77, 0x29, 0x3b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x6f, 0x75, // ord1).w);.    ou
	0x74, 0x2e, 0x62, 0x67, 0x66, 0x78, 0x5f, 0x46, 0x72, 0x61, 0x67, 0x44, 0x61, 0x74, 0x61, 0x30, // t.bgfx_FragData0
	0x20, 0x3d, 0x20, 0x5f, 0x32, 0x30, 0x36, 0x3b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x72, 0x65, 0x74, //  = _206;.    ret
	0x75, 0x72, 0x6e, 0x20, 0x6f, 0x75, 0x74, 0x3b, 0x0a, 0x7d, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x00, // urn out;.}......
};

extern const uint8_t* fs_tile_pssl;
extern const uint32_t fs_tile_pssl_size;
