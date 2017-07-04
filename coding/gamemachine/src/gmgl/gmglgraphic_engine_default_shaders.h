﻿BEGIN_NS

namespace gmgl_shaders
{
	struct object
	{
		const GMString vert =
			"#version 330\n"
			"\n"
			"uniform mat4 GM_shadow_matrix;\n"
			"uniform mat4 GM_view_matrix;\n"
			"uniform mat4 GM_model_matrix;\n"
			"uniform mat4 GM_projection_matrix;\n"
			"\n"
			"layout(location = 0) in vec4 position;\n"
			"layout(location = 1) in vec4 normal;\n"
			"layout(location = 2) in vec2 uv;\n"
			"layout(location = 3) in vec4 tangent;\n"
			"layout(location = 4) in vec4 bitangent;\n"
			"layout(location = 5) in vec2 lightmapuv;\n"
			"\n"
			"out vec4 shadowCoord;\n"
			"out vec4 _normal;\n"
			"out vec2 _uv;\n"
			"out vec4 _tangent;\n"
			"out vec4 _bitangent;\n"
			"out vec2 _lightmapuv;\n"
			"out vec4 position_world;\n"
			"\n"
			"void calcCoords()\n"
			"{\n"
			"	position_world = GM_model_matrix * position;\n"
			"	vec4 position_eye = GM_view_matrix * position_world;\n"
			"	gl_Position = GM_projection_matrix * position_eye;\n"
			"	shadowCoord = GM_shadow_matrix * position_world;\n"
			"	_normal = normal;\n"
			"	_tangent = tangent;\n"
			"	_bitangent = bitangent;\n"
			"	_uv = uv;\n"
			"	_lightmapuv = lightmapuv;\n"
			"}\n"
			"\n"
			"void main(void)\n"
			"{\n"
			"	calcCoords();\n"
			"}\n"
			;

		const GMString frag =
			"#version 330 core\n"
			"\n"
			"in vec4 position_world;\n"
			"in vec4 _normal;\n"
			"in vec2 _uv;\n"
			"in vec4 _tangent;\n"
			"in vec4 _bitangent;\n"
			"in vec2 _lightmapuv;\n"
			"in vec4 shadowCoord;\n"
			"\n"
			"// 阴影纹理\n"
			"uniform sampler2DShadow GM_shadow_texture;\n"
			"uniform int GM_shadow_texture_switch = 0;\n"
			"\n"
			"// 受环境系数影响的纹理 (Ka)\n"
			"uniform sampler2D GM_ambient_texture;\n"
			"uniform float GM_ambient_texture_scroll_s;\n"
			"uniform float GM_ambient_texture_scroll_t;\n"
			"uniform float GM_ambient_texture_scale_s;\n"
			"uniform float GM_ambient_texture_scale_t;\n"
			"uniform int GM_ambient_texture_switch = 0;\n"
			"\n"
			"uniform sampler2D GM_ambient_texture_2;\n"
			"uniform float GM_ambient_texture_2_scroll_s;\n"
			"uniform float GM_ambient_texture_2_scroll_t;\n"
			"uniform float GM_ambient_texture_2_scale_s;\n"
			"uniform float GM_ambient_texture_2_scale_t;\n"
			"uniform int GM_ambient_texture_2_switch = 0;\n"
			"\n"
			"uniform sampler2D GM_ambient_texture_3;\n"
			"uniform float GM_ambient_texture_3_scroll_s;\n"
			"uniform float GM_ambient_texture_3_scroll_t;\n"
			"uniform float GM_ambient_texture_3_scale_s;\n"
			"uniform float GM_ambient_texture_3_scale_t;\n"
			"uniform int GM_ambient_texture_3_switch = 0;\n"
			"\n"
			"// 受漫反射系数影响的纹理 (kd)\n"
			"uniform sampler2D GM_diffuse_texture;\n"
			"uniform int GM_diffuse_texture_switch = 0;\n"
			"\n"
			"// 环境立方体纹理（反射天空时）\n"
			"uniform samplerCube GM_reflection_cubemap_texture;\n"
			"uniform int GM_reflection_cubemap_texture_switch = 0;\n"
			"\n"
			"// 法线贴图纹理\n"
			"uniform sampler2D GM_normal_mapping_texture;\n"
			"uniform int GM_normal_mapping_texture_switch = 0;\n"
			"\n"
			"// 光照贴图纹理\n"
			"uniform sampler2D GM_lightmap_texture;\n"
			"uniform int GM_lightmap_texture_switch = 0;\n"
			"\n"
			"uniform mat4 GM_view_matrix;\n"
			"uniform vec3 GM_light_ambient; //环境光强度\n"
			"uniform vec3 GM_light_ka; // 环境光反射率\n"
			"uniform vec3 GM_light_power; // 点光（漫反射光，镜面反射光）强度\n"
			"uniform vec3 GM_light_kd; // 漫反射率\n"
			"uniform vec3 GM_light_ks; // 镜面反射率\n"
			"uniform vec3 GM_light_position; // 点光源位置\n"
			"uniform float GM_light_shininess; // 镜面反射cos系数\n"
			"uniform mat4 GM_model_matrix;\n"
			"\n"
			"// 调试变量\n"
			"uniform int GM_debug_draw_normal;\n"
			"\n"
			"// 漫反射系数\n"
			"float g_diffuse;\n"
			"// 镜面反射系数\n"
			"float g_specular;\n"
			"// 相机视角法向量\n"
			"vec3 g_normal_eye;\n"
			"\n"
			"out vec4 frag_color;\n"
			"\n"
			"float calcuateShadeFactor(vec4 shadowCoord)\n"
			"{\n"
			"	if (GM_shadow_texture_switch == 0)\n"
			"		return 1;\n"
			"\n"
			"	float shadeFactor = 0.0;\n"
			"	shadeFactor += textureProjOffset(GM_shadow_texture, shadowCoord, ivec2(-1, -1));\n"
			"	shadeFactor += textureProjOffset(GM_shadow_texture, shadowCoord, ivec2(1, -1));\n"
			"	shadeFactor += textureProjOffset(GM_shadow_texture, shadowCoord, ivec2(-1, 1));\n"
			"	shadeFactor += textureProjOffset(GM_shadow_texture, shadowCoord, ivec2(1, 1));\n"
			"	shadeFactor /= 4;\n"
			"\n"
			"	return shadeFactor;\n"
			"}\n"
			"\n"
			"float shadeFactorFactor(float shadeFactor)\n"
			"{\n"
			"	return min(shadeFactor + 0.3, 1);\n"
			"}\n"
			"\n"
			"void calcDiffuseAndSpecular(vec3 lightDirection, vec3 eyeDirection, vec3 normal)\n"
			"{\n"
			"	vec3 N = normalize(normal);\n"
			"	vec3 L = normalize(lightDirection);\n"
			"\n"
			"	//diffuse:\n"
			"	{\n"
			"		g_diffuse = dot(L, N);\n"
			"		g_diffuse = clamp(g_diffuse, 0.0f, 1.0f);\n"
			"	}\n"
			"\n"
			"	// specular\n"
			"	{\n"
			"		vec3 V = normalize(eyeDirection);\n"
			"		vec3 R = reflect(-L, N);\n"
			"		float theta = dot(V, R);\n"
			"		g_specular = pow(theta, GM_light_shininess);\n"
			"		g_specular = clamp(g_specular, 0.0f, 1.0f);\n"
			"	}\n"
			"}\n"
			"\n"
			"void calcLights()\n"
			"{\n"
			"	vec4 vertex_eye = GM_view_matrix * position_world;\n"
			"	vec3 eyeDirection_eye = vec3(0,0,0) - vertex_eye.xyz;\n"
			"	vec3 lightPosition_eye = (GM_view_matrix * vec4(GM_light_position, 1)).xyz;\n"
			"	vec3 lightDirection_eye = lightPosition_eye + eyeDirection_eye;\n"
			"\n"
			"	// 由顶点变换矩阵计算法向量变换矩阵\n"
			"	mat4 normalModelTransform = transpose(inverse(GM_model_matrix));\n"
			"	mat4 normalEyeTransform = GM_view_matrix * normalModelTransform;\n"
			"\n"
			"	// normal的齐次向量最后一位必须位0，因为法线变换不考虑平移\n"
			"	g_normal_eye = normalize( (normalEyeTransform * vec4(_normal.xyz, 0)).xyz );\n"
			"\n"
			"	if (GM_normal_mapping_texture_switch == 0)\n"
			"	{\n"
			"		calcDiffuseAndSpecular(lightDirection_eye, eyeDirection_eye, g_normal_eye);\n"
			"	}\n"
			"	else\n"
			"	{\n"
			"		vec3 tangent_eye = normalize((normalEyeTransform * vec4(_tangent.xyz, 0)).xyz);\n"
			"		vec3 bitangent_eye = normalize((normalEyeTransform * vec4(_bitangent.xyz, 0)).xyz);\n"
			"		mat3 TBN = transpose(mat3(\n"
			"			tangent_eye,\n"
			"			bitangent_eye,\n"
			"			g_normal_eye.xyz\n"
			"		));\n"
			"\n"
			"		vec3 lightDirection_tangent = TBN * lightDirection_eye;\n"
			"		vec3 eyeDirection_tangent = TBN * eyeDirection_eye;\n"
			"		vec3 normal_tangent = texture(GM_normal_mapping_texture, _uv).rgb * 2.0 - 1.0;\n"
			"\n"
			"		calcDiffuseAndSpecular(lightDirection_tangent, eyeDirection_tangent, normal_tangent);\n"
			"		//DEBUG.rgb = _tangent.xyz;\n"
			"	}\n"
			"}\n"
			"\n"
			"void drawObject()\n"
			"{\n"
			"	calcLights();\n"
			"\n"
			"	if (GM_debug_draw_normal == 1)\n"
			"	{\n"
			"		// 画眼睛视角的法向量\n"
			"		frag_color = vec4((g_normal_eye.xyz + 1.f) / 2.f, 1.f);\n"
			"		return;\n"
			"	}\n"
			"	else if (GM_debug_draw_normal == 2)\n"
			"	{\n"
			"		// 画世界视觉的法向量\n"
			"		frag_color = vec4((_normal.xyz + 1.f) / 2.f, 1.f);\n"
			"		return;\n"
			"	}\n"
			"\n"
			"	// 计算阴影系数\n"
			"	float shadeFactor = shadeFactorFactor(calcuateShadeFactor(shadowCoord));\n"
			"\n"
			"	// 反射光\n"
			"	vec3 diffuseTextureColor = GM_diffuse_texture_switch == 1 ? vec3(texture(GM_diffuse_texture, _uv)) : vec3(1);\n"
			"	vec3 diffuseLight =\n"
			"		// 漫反射光系数\n"
			"		g_diffuse *\n"
			"		// 漫光反射率\n"
			"		GM_light_kd *\n"
			"		// ShadowMap的阴影系数，如果没有ShadowMap则为1\n"
			"		shadeFactor *\n"
			"		// 漫反射纹理\n"
			"		diffuseTextureColor *\n"
			"		// 镜面光强度\n"
			"		GM_light_power;\n"
			"\n"
			"	vec3 specularLight =\n"
			"		// 镜面反射系数\n"
			"		g_specular *\n"
			"		// 镜面反射率\n"
			"		GM_light_ks *\n"
			"		// ShadowMap的阴影系数，如果没有ShadowMap则为1\n"
			"		shadeFactor *\n"
			"		// 镜面光强度\n"
			"		GM_light_power;\n"
			"\n"
			"	// 计算环境光和Ka贴图\n"
			"	vec3 ambientTextureColor = GM_ambient_texture_switch == 1 ? vec3(texture(GM_ambient_texture, _uv * vec2(GM_ambient_texture_scale_s, GM_ambient_texture_scale_t) + vec2(GM_ambient_texture_scroll_s, GM_ambient_texture_scroll_t))) : vec3(1);\n"
			"	ambientTextureColor += GM_ambient_texture_2_switch == 1 ? vec3(texture(GM_ambient_texture_2, _uv * vec2(GM_ambient_texture_2_scale_s, GM_ambient_texture_2_scale_t) + vec2(GM_ambient_texture_2_scroll_s, GM_ambient_texture_2_scroll_t))) : vec3(0);\n"
			"	ambientTextureColor += GM_ambient_texture_3_switch == 1 ? vec3(texture(GM_ambient_texture_3, _uv * vec2(GM_ambient_texture_3_scale_s, GM_ambient_texture_3_scale_t) + vec2(GM_ambient_texture_3_scroll_s, GM_ambient_texture_3_scroll_t))) : vec3(0);\n"
			"	ambientTextureColor *= GM_lightmap_texture_switch == 1 ? vec3(texture(GM_lightmap_texture, _lightmapuv)) : vec3(1);\n"
			"\n"
			"	// 环境光\n"
			"	vec3 ambientLight =\n"
			"		// 环境光反射率\n"
			"		GM_light_ka *\n"
			"		// 环境光强度\n"
			"		GM_light_ambient*\n"
			"		// ShadowMap的阴影系数，如果没有ShadowMap则为1\n"
			"		shadeFactor *\n"
			"		// 环境光纹理\n"
			"		ambientTextureColor;\n"
			"\n"
			"	// 最终结果\n"
			"	vec3 color = ambientLight + diffuseLight + specularLight;\n"
			"	frag_color = vec4(color, 1.0f);\n"
			"}\n"
			"\n"
			"void main()\n"
			"{\n"
			"	drawObject();\n"
			"}\n"
			;
	} object;

	struct glyph
	{
		const GMString vert =
			"#version 330\n"
			"\n"
			"layout (location = 0) in vec4 position;\n"
			"layout (location = 2) in vec2 uv;\n"
			"out vec2 _uv;\n"
			"\n"
			"void main(void)\n"
			"{\n"
			"	gl_Position = position;\n"
			"	_uv = uv;\n"
			"}\n"
			;
			
		const GMString frag =
			"#version 330 core\n"
			"\n"
			"uniform sampler2D GM_ambient_texture;\n"
			"uniform float GM_ambient_texture_scroll_s;\n"
			"uniform float GM_ambient_texture_scroll_t;\n"
			"uniform float GM_ambient_texture_scale_s;\n"
			"uniform float GM_ambient_texture_scale_t;\n"
			"\n"
			"in vec2 _uv;\n"
			"out vec4 frag_color;\n"
			"\n"
			"void main()\n"
			"{\n"
			"	frag_color = texture(GM_ambient_texture,\n"
			"		_uv * vec2(GM_ambient_texture_scale_s, GM_ambient_texture_scale_t) + vec2(GM_ambient_texture_scroll_s, GM_ambient_texture_scroll_t)\n"
			"	);\n"
			"}\n"
			;
	} glyph;

	struct sky
	{
		const GMString vert =
			"#version 330\n"
			"\n"
			"uniform mat4 GM_view_matrix;\n"
			"uniform mat4 GM_model_matrix;\n"
			"uniform mat4 GM_projection_matrix;\n"
			"\n"
			"layout (location = 0) in vec4 position;\n"
			"layout (location = 2) in vec2 uv;\n"
			"\n"
			"out vec2 _uv;\n"
			"\n"
			"void main(void)\n"
			"{\n"
			"	vec4 position_world = GM_model_matrix * position;\n"
			"	vec4 position_eye = GM_view_matrix * position_world;\n"
			"	gl_Position = GM_projection_matrix * position_eye;\n"
			"	_uv = uv;\n"
			"}\n"
			;

		const GMString frag =
			"#version 330 core\n"
			"\n"
			"uniform sampler2D GM_ambient_texture;\n"
			"uniform float GM_ambient_texture_scroll_s;\n"
			"uniform float GM_ambient_texture_scroll_t;\n"
			"uniform float GM_ambient_texture_scale_s;\n"
			"uniform float GM_ambient_texture_scale_t;\n"
			"uniform int GM_ambient_texture_switch = 0;\n"
			"\n"
			"uniform sampler2D GM_ambient_texture_2;\n"
			"uniform float GM_ambient_texture_2_scroll_s;\n"
			"uniform float GM_ambient_texture_2_scroll_t;\n"
			"uniform float GM_ambient_texture_2_scale_s;\n"
			"uniform float GM_ambient_texture_2_scale_t;\n"
			"uniform int GM_ambient_texture_2_switch = 0;\n"
			"\n"
			"uniform sampler2D GM_ambient_texture_3;\n"
			"uniform float GM_ambient_texture_3_scroll_s;\n"
			"uniform float GM_ambient_texture_3_scroll_t;\n"
			"uniform float GM_ambient_texture_3_scale_s;\n"
			"uniform float GM_ambient_texture_3_scale_t;\n"
			"uniform int GM_ambient_texture_3_switch = 0;\n"
			"\n"
			"uniform vec3 GM_light_ambient;\n"
			"\n"
			"in vec2 _uv;\n"
			"out vec4 frag_color;\n"
			"\n"
			"void drawSky()\n"
			"{\n"
			"	vec3 color = GM_light_ambient * vec3(texture(GM_ambient_texture, _uv * vec2(GM_ambient_texture_scale_s, GM_ambient_texture_scale_t) + vec2(GM_ambient_texture_scroll_s, GM_ambient_texture_scroll_t)));\n"
			"	color += GM_ambient_texture_2_switch == 1 ? vec3(texture(GM_ambient_texture_2, _uv * vec2(GM_ambient_texture_2_scale_s, GM_ambient_texture_2_scale_t) + vec2(GM_ambient_texture_2_scroll_s, GM_ambient_texture_2_scroll_t))) : vec3(0);\n"
			"	color += GM_ambient_texture_3_switch == 1 ? vec3(texture(GM_ambient_texture_3, _uv * vec2(GM_ambient_texture_3_scale_s, GM_ambient_texture_3_scale_t) + vec2(GM_ambient_texture_3_scroll_s, GM_ambient_texture_3_scroll_t))) : vec3(0);\n"
			"	frag_color = vec4(color, 1.0f);\n"
			"}\n"
			"\n"
			"void main()\n"
			"{\n"
			"	drawSky();\n"
			"}\n"
			;
	} sky;
}
END_NS