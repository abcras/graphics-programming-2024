//Inputs
in vec2 TexCoord;

//Outputs
out vec4 FragColor;

//Uniforms
uniform sampler2D DepthTexture;
uniform sampler2D AlbedoTexture;
uniform sampler2D NormalTexture;
uniform sampler2D OthersTexture;
uniform mat4 InvViewMatrix;
uniform mat4 InvProjMatrix;

void main()
{
	//FragColor = vec4(texture(AlbedoTexture, TexCoord));
	//FragColor = vec4(GetImplicitNormal(texture(NormalTexture, TexCoord).xy), 1.0f);
	//FragColor = vec4(ReconstructViewPosition(DepthTexture, TexCoord, InvProjMatrix), 1.0f);

	SurfaceData data;
	data.normal = (vec4(GetImplicitNormal(texture(NormalTexture, TexCoord).xy), 0.0f) * InvViewMatrix).rgb;
	data.reflectionColor = texture(AlbedoTexture, TexCoord).rgb;

	vec4 others = texture(OthersTexture, TexCoord);
	//vec4(AmbientReflectance, DiffuseReflectance, SpecularReflectance, 1/(1+SpecularExponent));
	float w = others.w;
	w = 1/(w) -1;

	data.ambientReflectance = others.x;
	data.diffuseReflectance = others.y;
	data.specularReflectance = others.z;
	data.specularExponent = w;

	vec3 position = ReconstructViewPosition(DepthTexture, TexCoord, InvProjMatrix);
	vec3 viewDir = GetDirection(position, vec3(0.0f));

	vec3 positionWorld = (vec4(position, 1.0f) * InvViewMatrix).xyz;
	vec3 viewDirWorld = (vec4(viewDir, 0.0f) * InvViewMatrix).xyz;

	vec3 color = ComputeLighting(positionWorld, data, viewDirWorld, true);
	FragColor = vec4(color.rgb, 1);
}
