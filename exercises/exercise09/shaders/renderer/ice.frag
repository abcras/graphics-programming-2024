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

uniform sampler2D IceColorTexture;
uniform sampler2D IceCombinedTexture;
//R is subsurface
//G is roughness
//B is Ambient occlusion

uniform vec3 CameraPosition;

void main()
{
	SurfaceData data;
	data.normal = SampleNormalMap(NormalTexture, TexCoord, normalize(WorldNormal), normalize(WorldTangent), normalize(WorldBitangent));
	//vec4 colorTexture = texture(ColorTexture, TexCoord);
	vec4 colorTexture = texture(IceColorTexture, TexCoord);


	data.albedo = Color * colorTexture.rgb;
	//vec3 arm = texture(SpecularTexture, TexCoord).rgb;
	vec3 sra = texture(IceCombinedTexture, TexCoord).rgb;
	data.ambientOcclusion = sra.b;
	data.roughness = sra.g;
	data.metalness = sra.r;

	vec3 position = WorldPosition;
	vec3 viewDir = GetDirection(position, CameraPosition);
	vec3 color = ComputeLighting(position, data, viewDir, true);
	FragColor = vec4(color.rgb, colorTexture.a);
}
