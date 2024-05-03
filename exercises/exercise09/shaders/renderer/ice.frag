//Inputs
in vec3 WorldPosition;
in vec3 WorldNormal;
in vec3 WorldTangent;
in vec3 WorldBitangent;
in vec2 TexCoord;

//Outputs
out vec4 FragColor;

//Uniforms
uniform vec3 Color;
uniform sampler2D ColorTexture;
uniform sampler2D NormalTexture;
uniform sampler2D SpecularTexture;

uniform float Time;

uniform sampler2D IceColorTexture;
uniform sampler2D IceCombinedTexture;
//R is subsurface        X
//G is roughness         Y
//B is Ambient occlusion Z

uniform vec3 CameraPosition;
uniform vec3 IceEpiCenter;

void main()
{
	SurfaceData data;
	data.normal = SampleNormalMap(NormalTexture, TexCoord, normalize(WorldNormal), normalize(WorldTangent), normalize(WorldBitangent));
	vec4 defaultTexture = texture(ColorTexture, TexCoord);
	vec4 colorTexture = texture(IceColorTexture, TexCoord);
	

	data.albedo = Color * colorTexture.rgb;
	//data.albedo = Color * defaultTexture.rgb;

	vec3 arm = texture(SpecularTexture, TexCoord).rgb;
	vec3 sra = texture(IceCombinedTexture, TexCoord).rgb;
	data.ambientOcclusion = sra.b;
	data.roughness = sra.g;
	data.metalness = arm.z;
	data.subsurface = sra.r;

	vec3 position = WorldPosition;
	vec3 viewDir = GetDirection(position, CameraPosition);
	vec3 color = ComputeLighting(position, data, viewDir, true);
	//color.b = color.b + Time;
	//FragColor = vec4(color.rgb, 1.0f);

	//FragColor = vec4(color.rgb, defaultTexture.a);

	FragColor = vec4(color.rgb, 0.9f);
}
