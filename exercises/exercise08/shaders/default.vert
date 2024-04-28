//Inputs
layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec3 VertexTangent;
layout (location = 3) in vec3 VertexBitangent;
layout (location = 4) in vec2 VertexTexCoord;

//Outputs
out vec3 WorldPosition;
out vec3 WorldNormal;
out vec3 WorldTangent;
out vec3 WorldBitangent;
out vec2 TexCoord;

out float SpecularIntensity;
out vec3 v_eyeDirectModel;

//Uniforms
uniform mat4 WorldMatrix;
uniform mat4 ViewProjMatrix;

float GetSpecularIntensity(vec4 position, vec3 a_normal, vec3 eyeDirectModel) {
    float shininess = 30.0f;
    vec3 lightPosition = vec3(-20.0f, 0.0f, 0.0f);
    // We ignore that N dot L could be negative (light coming from behind the surface)
    vec3 LightDirModel = normalize(lightPosition - position.xyz);
    vec3 halfVector = normalize(LightDirModel + eyeDirectModel);
    float NdotH = max(dot(a_normal, halfVector), 0.0f);
    return pow(NdotH, shininess);
}

void main()
{
	// vertex position in world space (for lighting computation)
	WorldPosition = (WorldMatrix * vec4(VertexPosition, 1.0f)).xyz;

	v_eyeDirectModel = normalize(-WorldPosition.xyz);

	// normal in world space (for lighting computation)
	WorldNormal = (WorldMatrix * vec4(VertexNormal, 0.0f)).xyz;


	//I put the function here becuase it kept on not working.
	float shininess = 30.0f;
    vec3 lightPosition = vec3(-20.0f, 0.0f, 0.0f);
    // We ignore that N dot L could be negative (light coming from behind the surface)
    vec3 LightDirModel = normalize(lightPosition - WorldPosition.xyz);
    vec3 halfVector = normalize(LightDirModel + v_eyeDirectModel);
    float NdotH = max(dot(WorldNormal, halfVector), 0.0f);
    SpecularIntensity = pow(NdotH, shininess);
	
	//SpecularIntensity = GetSpecularIntensity(WorldPosition, WorldNormal, v_eyeDirectModel);




	// tangent in world space (for lighting computation)
	WorldTangent = (WorldMatrix * vec4(VertexTangent, 0.0f)).xyz;

	// bitangent in world space (for lighting computation)
	WorldBitangent = (WorldMatrix * vec4(VertexBitangent, 0.0f)).xyz;

	// texture coordinates
	TexCoord = VertexTexCoord;

	// final vertex position (for opengl rendering, not for lighting)
	gl_Position = ViewProjMatrix * vec4(WorldPosition, 1.0f);
}
