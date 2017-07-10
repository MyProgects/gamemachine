#version 330 core
struct GM_texture_t
{
    sampler2D texture;
    float scroll_s;
    float scroll_t;
    float scale_s;
    float scale_t;
    int enabled;
};
uniform GM_texture_t GM_ambient_textures[1];

/*
uniform sampler2D GM_ambient_texture;
uniform float GM_ambient_texture_scroll_s;
uniform float GM_ambient_texture_scroll_t;
uniform float GM_ambient_texture_scale_s;
uniform float GM_ambient_texture_scale_t;
*/

in vec2 _uv;
out vec4 frag_color;

void main()
{
	frag_color = texture(GM_ambient_textures[0].texture, 
		_uv * vec2(GM_ambient_textures[0].scale_s, GM_ambient_textures[0].scale_t) + vec2(GM_ambient_textures[0].scroll_s, GM_ambient_textures[0].scroll_t)
	);
}
