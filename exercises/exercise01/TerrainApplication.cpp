#include "TerrainApplication.h"
#include <itugl/geometry/VertexAttribute.h>

// (todo) 01.1: Include the libraries you need

#include <cmath>
#include <iostream>
#include <vector>

// Helper structures. Declared here only for this exercise
struct Vector2
{
	Vector2() : Vector2(0.f, 0.f) {}
	Vector2(float x, float y) : x(x), y(y) {}
	float x, y;
};

struct Vector3
{
	Vector3() : Vector3(0.f, 0.f, 0.f) {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
	float x, y, z;

	Vector3 Normalize() const
	{
		float length = std::sqrt(1 + x * x + y * y);
		return Vector3(x / length, y / length, z / length);
	}
};

// (todo) 01.8: Declare an struct with the vertex format

// Helper struct with the vertex format
struct Vertex
{
	Vector3 position;
	Vector2 texCoord;
	Vector3 color;
	Vector3 normal;
};


TerrainApplication::TerrainApplication()
	: Application(1024, 1024, "Terrain demo"), m_gridX(16), m_gridY(16), m_shaderProgram(0)
{
}

void TerrainApplication::Initialize()
{
	Application::Initialize();

	// Build shaders and store in m_shaderProgram
	BuildShaders();

	// (todo) 01.1: Create containers for the vertex position
	std::vector<Vector3> vertices;

	unsigned int columnCount = m_gridX + 1;
	unsigned int rowCount = m_gridY + 1;

	Vector2 scale(1.0f / m_gridX, 1.0f / m_gridY);

	// (todo) 01.1: Fill in vertex data
	for (unsigned int j = 0u; j < rowCount; ++j)
	{
		for (unsigned int i = 0u; i < columnCount; ++i)
		{
			float x = i * scale.x - 0.5f;
			float y = j * scale.y - 0.5f;
			vertices.push_back(Vector3(x - 0.5f * scale.x, y + 0.5f * scale.y, 0u));
			vertices.push_back(Vector3(x + 0.5f * scale.x, y + 0.5f * scale.y, 0u));
			vertices.push_back(Vector3(x - 0.5f * scale.x, y - 0.5f * scale.y, 0u));
			vertices.push_back(Vector3(x + 0.5f * scale.x, y + 0.5f * scale.y, 0u));
			vertices.push_back(Vector3(x + 0.5f * scale.x, y - 0.5f * scale.y, 0u));
			vertices.push_back(Vector3(x - 0.5f * scale.x, y - 0.5f * scale.y, 0u));
		}
	}
	//VertexAttribute positionAttribute(Data::Type::Float, 3);

	m_vbo.Bind();
	m_vbo.AllocateData(std::span(vertices));

	//VertexAttribute positionAttribute = new VertexAttribute(Data::Type::Float, 3, false);
	VertexAttribute positionAttribute(Data::Type::Float, 3, false);

	size_t positionOffset = 0u;

	GLsizei stride = sizeof(positionAttribute);
	//auto position = VertexAttribute(Data::Data(GluShort.Float),)

	m_vao.Bind();
	m_vao.SetAttribute(0, positionAttribute, static_cast<GLint>(positionOffset), stride);
	//m_vao.AllocatedData

	// (todo) 01.1: Initialize VAO, and VBO


	// (todo) 01.5: Initialize EBO


	// (todo) 01.1: Unbind VAO, and VBO

	VertexBufferObject::Unbind();
	VertexArrayObject::Unbind();

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


	// (todo) 01.5: Unbind EBO

}

void TerrainApplication::Update()
{
	Application::Update();

	UpdateOutputMode();
}

void TerrainApplication::Render()
{
	Application::Render();

	// Clear color and depth
	GetDevice().Clear(true, Color(0.0f, 0.0f, 0.0f, 1.0f), true, 1.0f);

	// Set shader to be used
	glUseProgram(m_shaderProgram);

	// (todo) 01.1: Draw the grid
	m_vao.Bind();


	//correct amount of triangles for exercise 01.3
	//glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glDrawArrays(GL_TRIANGLES, 0, (m_gridX+1) * (m_gridY+1)*6);

}

void TerrainApplication::Cleanup()
{
	Application::Cleanup();
}


void TerrainApplication::BuildShaders()
{
	const char* vertexShaderSource = "#version 330 core\n"
		"layout (location = 0) in vec3 aPos;\n"
		"layout (location = 1) in vec2 aTexCoord;\n"
		"layout (location = 2) in vec3 aColor;\n"
		"layout (location = 3) in vec3 aNormal;\n"
		"uniform mat4 Matrix = mat4(1);\n"
		"out vec2 texCoord;\n"
		"out vec3 color;\n"
		"out vec3 normal;\n"
		"void main()\n"
		"{\n"
		"   texCoord = aTexCoord;\n"
		"   color = aColor;\n"
		"   normal = aNormal;\n"
		"   gl_Position = Matrix * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
		"}\0";
	const char* fragmentShaderSource = "#version 330 core\n"
		"uniform uint Mode = 0u;\n"
		"in vec2 texCoord;\n"
		"in vec3 color;\n"
		"in vec3 normal;\n"
		"out vec4 FragColor;\n"
		"void main()\n"
		"{\n"
		"   switch (Mode)\n"
		"   {\n"
		"   default:\n"
		"   case 0u:\n"
		"       FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);\n"
		"       break;\n"
		"   case 1u:\n"
		"       FragColor = vec4(fract(texCoord), 0.0f, 1.0f);\n"
		"       break;\n"
		"   case 2u:\n"
		"       FragColor = vec4(color, 1.0f);\n"
		"       break;\n"
		"   case 3u:\n"
		"       FragColor = vec4(normalize(normal), 1.0f);\n"
		"       break;\n"
		"   case 4u:\n"
		"       FragColor = vec4(color * max(dot(normalize(normal), normalize(vec3(1,0,1))), 0.2f), 1.0f);\n"
		"       break;\n"
		"   }\n"
		"}\n\0";

	// vertex shader
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// check for shader compile errors
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// fragment shader
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// check for shader compile errors
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// link shaders
	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// check for linking errors
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	m_shaderProgram = shaderProgram;
}

void TerrainApplication::UpdateOutputMode()
{
	for (int i = 0; i <= 4; ++i)
	{
		if (GetMainWindow().IsKeyPressed(GLFW_KEY_0 + i))
		{
			int modeLocation = glGetUniformLocation(m_shaderProgram, "Mode");
			glUseProgram(m_shaderProgram);
			glUniform1ui(modeLocation, i);
			break;
		}
	}
	if (GetMainWindow().IsKeyPressed(GLFW_KEY_TAB))
	{
		const float projMatrix[16] = { 0, -1.294f, -0.721f, -0.707f, 1.83f, 0, 0, 0, 0, 1.294f, -0.721f, -0.707f, 0, 0, 1.24f, 1.414f };
		int matrixLocation = glGetUniformLocation(m_shaderProgram, "Matrix");
		glUseProgram(m_shaderProgram);
		glUniformMatrix4fv(matrixLocation, 1, false, projMatrix);
	}
}
