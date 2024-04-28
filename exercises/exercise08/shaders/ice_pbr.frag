//Inputs
in vec3 WorldPosition;
in vec3 WorldNormal;
in vec3 WorldTangent;
in vec3 WorldBitangent;
in vec2 TexCoord;

in float SpecularIntensity;
in vec3 v_eyeDirectModel;

//Outputs
out vec4 FragColor;

//Uniforms
uniform vec3 Color;
uniform sampler2D ColorTexture;
uniform sampler2D NormalTexture;
uniform sampler2D SpecularTexture;
uniform mat4 WorldMatrix;

uniform vec3 CameraPosition;
vec3 CenterOfUniverse = vec3(0.0f);
uniform float Time;

void main()
{
	SurfaceData data;
	data.normal = SampleNormalMap(NormalTexture, TexCoord, normalize(WorldNormal), normalize(WorldTangent), normalize(WorldBitangent));
	data.albedo = Color * texture(ColorTexture, TexCoord).rgb;
	vec3 arm = texture(SpecularTexture, TexCoord).rgb;
	data.ambientOcclusion = arm.x;
	data.roughness = arm.y;
	data.metalness = arm.z;
	
	float dist = GetDistance(WorldPosition, CenterOfUniverse);
	
	float longDist = dist * 50;

	float locTime = Time / 50;// * (data.metalness + 0.2);
	

	//dist -= data.roughness;

	if(longDist < Time){
		
		data.roughness = mix(arm.y, 0.2f * arm.y, locTime - dist);
		data.metalness = mix(arm.z, 0.0f * arm.z, locTime - dist);
	}

	
	vec3 position = WorldPosition;
	vec3 viewDir = GetDirection(position, CameraPosition);
	vec3 color = ComputeLighting(position, data, viewDir, true);

	//mix()
	if(longDist < Time){
		color.b = mix( color.b, color.b * 10.0f, locTime - dist);
	}


	vec4 temp = vec4(color.rgb, 1.0f);
	//temp.a = 0.1f;
	FragColor = temp;
}
