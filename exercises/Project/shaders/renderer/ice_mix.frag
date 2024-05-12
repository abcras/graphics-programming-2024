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
uniform vec2 IceSamplingScale;

uniform sampler2D PerlinNoiseTexture;
uniform sampler2D VoronoiNoiseTexture;

uniform int DebugMode;

vec2 GetDistances(float _time){
	
	float d1, d2;

	//define the ice growing function:
	//time is in seconds using float
	//distance is in units described using float
	// d1 should grow slower than d2

	d1 = _time / 10;
	d2 = d1 * 2.5f;

	return vec2(d1,d2);
}


void main()
{
	SurfaceData data;
	data.normal = SampleNormalMap(NormalTexture, TexCoord, normalize(WorldNormal), normalize(WorldTangent), normalize(WorldBitangent));
	vec4 defaultTexture = texture(ColorTexture, TexCoord);
	vec4 iceTexture = texture(IceColorTexture, TexCoord);
	


	float dist = GetDistance(IceEpiCenter, WorldPosition);
	vec2 iceGrowth = GetDistances(Time);
	float ratio = 0.f;

	vec3 position = WorldPosition;
	vec3 viewDir = GetDirection(position, CameraPosition);

	vec2 perlinTexture = texture(SpecularTexture, TexCoord).xy;

	iceGrowth.x += perlinTexture.x;
	iceGrowth.y += perlinTexture.y;

	vec3 color = vec3(0);

	vec3 DebugColorHelper = vec3(0);

	ratio = invLerp(iceGrowth.y, iceGrowth.x, dist);

	ratio = clamp(ratio, 0.00000001f, 1.f);


	vec3 iceAlbedo = Color * iceTexture.rgb;
		
	vec3 defaultAlbedo = Color * defaultTexture.rgb;

	data.normal *= mix(vec3(1), normalize(texture(VoronoiNoiseTexture, TexCoord * IceSamplingScale)).xyz, ratio);


	vec3 arm = texture(SpecularTexture, TexCoord).rgb;
	vec3 sra = texture(IceCombinedTexture, TexCoord).rgb;
	data.ambientOcclusion = mix(arm.x, sra.b, ratio);
	data.roughness = mix(arm.y, sra.g, ratio);
	data.metalness = mix(arm.z, 0.002, ratio);
	data.subsurface = mix(0.0000001, sra.r, ratio);

	data.albedo = defaultAlbedo;
	vec3 color1 = ComputeLighting(position, data, viewDir, true);
	data.albedo = iceAlbedo;
	vec3 color2 = ComputeLighting(position, data, viewDir, true);

	DebugColorHelper.r = mix (0,1,1-ratio);
	DebugColorHelper.b = mix (0,1,ratio);
	color = mix (color1, color2, ratio);


	color.b += ratio;

	vec3 fresnel = mix(vec3(0), FresnelSchlick(GetReflectance(data), viewDir, data.normal), ratio);


	FragColor = vec4(mix(color.rgb + fresnel, DebugColorHelper, float(DebugMode)), 0.9f);
}
