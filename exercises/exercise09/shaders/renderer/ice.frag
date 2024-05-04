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

vec2 GetDistances(float _time){
	
	float d1, d2;

	//define the ice growing function:
	//time is in seconds using float
	//distance is in units described using float
	// d1 should grow slower than d2

	d1 = _time / 10;
	d2 = d1 * 1.5f;

	return vec2(d1,d2);
}


void main()
{

	bool DebugMode = false;

	SurfaceData data;
	data.normal = SampleNormalMap(NormalTexture, TexCoord, normalize(WorldNormal), normalize(WorldTangent), normalize(WorldBitangent));
	vec4 defaultTexture = texture(ColorTexture, TexCoord);
	vec4 iceTexture = texture(IceColorTexture, TexCoord);
	
	float dist = GetDistance(IceEpiCenter, WorldPosition);
	vec2 iceGrowth = GetDistances(Time);
	float ratio = 0;

	vec3 DebugColorHelper = vec3(0);
	if(dist >= iceGrowth.y)
	{
		//normal texture / no ice 
		data.albedo = Color * defaultTexture.rgb;

		vec3 arm = texture(SpecularTexture, TexCoord).rgb;
		vec3 sra = texture(IceCombinedTexture, TexCoord).rgb;
		data.ambientOcclusion = arm.x;
		data.roughness = arm.r;
		data.metalness = arm.z;
		data.subsurface = sra.r;
		DebugColorHelper.x = 1;
	}
	if (dist > iceGrowth.x && dist < iceGrowth.y)
	{
		//use mix with dist to get inbetween values of normal / ice texture
		//y is max and x is minimum
		ratio = invLerp(iceGrowth.x, iceGrowth.y, dist);
		vec3 iceAlbedo = Color * iceTexture.rgb;
		
		vec3 defaultAlbedo = Color * defaultTexture.rgb;

		data.albedo = mix(defaultAlbedo, iceAlbedo, ratio);

		vec3 arm = texture(SpecularTexture, TexCoord).rgb;
		vec3 sra = texture(IceCombinedTexture, TexCoord).rgb;
		data.ambientOcclusion = mix(arm.x, sra.b, ratio);
		data.roughness = mix(arm.y, sra.g, ratio);
		data.metalness = mix(arm.z, 0.0000001, ratio);
		data.subsurface = mix(0.0000001, sra.r, ratio);
		DebugColorHelper.y = 1;
	}
	if ( dist <= iceGrowth.x)
	{
		ratio = 1;
		//if x (which is the shorter distance) is bigger than the distance
		//Fully ice
		data.albedo = data.albedo = Color * iceTexture.rgb;
		vec3 arm = texture(SpecularTexture, TexCoord).rgb;
		vec3 sra = texture(IceCombinedTexture, TexCoord).rgb;

		data.ambientOcclusion = sra.b;
		data.roughness = sra.g;
		data.metalness = arm.z;
		data.subsurface = sra.r;
		DebugColorHelper.z = 1;
	}

	vec3 position = WorldPosition;
	vec3 viewDir = GetDirection(position, CameraPosition);
	vec3 color = ComputeLighting(position, data, viewDir, true);
	color.b += ratio;
	//FragColor = vec4(color.rgb, 1.0f);

	//FragColor = vec4(color.rgb, defaultTexture.a);
	
	if(DebugMode)
	{
		FragColor = vec4(DebugColorHelper, 0.9f);
	} else
	{
		FragColor = vec4(color.rgb, 0.9f);
	}
	
}
