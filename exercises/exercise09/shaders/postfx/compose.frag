//Inputs
in vec2 TexCoord;

//Outputs
out vec4 FragColor;

//Uniforms
uniform sampler2D SourceTexture;
uniform float Exposure;

void main()
{
	vec4 colorHDR = texture(SourceTexture, TexCoord);
	FragColor = 1- exp(-colorHDR * Exposure);
}
