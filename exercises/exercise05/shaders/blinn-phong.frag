#version 330 core

in vec3 WorldPosition;
in vec3 WorldNormal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec4 Color;
uniform sampler2D ColorTexture;
uniform vec3 AmbientColor;
uniform float AmbientReflection;

vec3 GetAmbientReflection(vec3 objectColor)
{
	return AmbientColor * AmbientReflection * objectColor;
}

vec3 GetBlinnPhongReflection(vec3 objectColor)
{
	return GetAmbientReflection(objectColor);
}

void main()
{
	vec4 objectColor = Color * texture(ColorTexture, TexCoord);

	FragColor = vec4(GetBlinnPhongReflection(objectColor.rgb), 1.f);
}
