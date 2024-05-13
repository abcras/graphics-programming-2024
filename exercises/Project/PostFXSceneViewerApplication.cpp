#include "PostFXSceneViewerApplication.h"

#include <ituGL/asset/TextureCubemapLoader.h>
#include <ituGL/asset/ShaderLoader.h>
#include <ituGL/asset/ModelLoader.h>

#include <ituGL/camera/Camera.h>
#include <ituGL/scene/SceneCamera.h>

#include <ituGL/lighting/DirectionalLight.h>
#include <ituGL/lighting/PointLight.h>
#include <ituGL/scene/SceneLight.h>

#include <ituGL/shader/ShaderUniformCollection.h>
#include <ituGL/shader/Material.h>
#include <ituGL/geometry/Model.h>
#include <ituGL/scene/SceneModel.h>

#include <ituGL/renderer/SkyboxRenderPass.h>
#include <ituGL/renderer/GBufferRenderPass.h>
#include <ituGL/renderer/DeferredRenderPass.h>
#include <ituGL/renderer/ForwardRenderPass.h>
#include <ituGL/renderer/PostFXRenderPass.h>
#include <ituGL/scene/RendererSceneVisitor.h>

#include <ituGL/scene/Transform.h>
#include <ituGL/scene/ImGuiSceneVisitor.h>
#include <imgui.h>

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

PostFXSceneViewerApplication::PostFXSceneViewerApplication()
	: Application(1024, 1024, "Post FX Scene Exam Project version")
	, m_renderer(GetDevice())
	, m_sceneFramebuffer(std::make_shared<FramebufferObject>())
	, m_exposure(1.0f)
	, m_contrast(1.1f)
	, m_hueShift(0.0f)
	, m_saturation(1.0f)
	, m_colorFilter(1.0f)
	, m_blurIterations(1)
	, m_bloomRange(1.0f, 2.0f)
	, m_bloomIntensity(2.0f)
	, m_transparentCollection(0)
	, m_time(0)
	, m_iceEpicenter(0)
	, m_debugMode(false)
	, m_iceSamplingScale(1, 6)
	, m_timeScale(1)
{
}

void PostFXSceneViewerApplication::Initialize()
{
	Application::Initialize();

	//m_iceSamplingScale.y = 6.f;

	// Initialize DearImGUI
	m_imGui.Initialize(GetMainWindow());

	InitializeCamera();
	InitializeLights();
	InitializeMaterials();
	InitializeModels();
	InitializeRenderer();
}

void PostFXSceneViewerApplication::Update()
{
	Application::Update();


	m_time += GetDeltaTime() * m_timeScale;
	// Update camera controller
	m_cameraController.Update(GetMainWindow(), GetDeltaTime());

	//DEBUGGIN TOOL: visualises the growth of the effect.
	if (m_debugMode)
	{
		epicenterModel->GetTransform()->SetScale(glm::vec3(m_time / 10));
		epicenterModel->GetTransform()->SetTranslation(m_iceEpicenter);
	}

	// Add the scene nodes to the renderer
	RendererSceneVisitor rendererSceneVisitor(m_renderer);
	m_scene.AcceptVisitor(rendererSceneVisitor);

	// Sort transparent pass
	using namespace std::placeholders;
	m_renderer.SortDrawcallCollection(m_transparentCollection, std::bind(&Renderer::IsBackToFront, &m_renderer, _1, _2));
}

void PostFXSceneViewerApplication::Render()
{
	Application::Render();

	GetDevice().Clear(true, Color(0.0f, 0.0f, 0.0f, 1.0f), true, 1.0f);

	// Render the scene
	m_renderer.Render();

	// Render the debug user interface
	RenderGUI();
}

void PostFXSceneViewerApplication::Cleanup()
{
	// Cleanup DearImGUI
	m_imGui.Cleanup();

	Application::Cleanup();
}

void PostFXSceneViewerApplication::InitializeCamera()
{
	// Create the main camera
	std::shared_ptr<Camera> camera = std::make_shared<Camera>();
	camera->SetViewMatrix(glm::vec3(-2, 1, -2), glm::vec3(0, 0.5f, 0), glm::vec3(0, 1, 0));
	camera->SetPerspectiveProjectionMatrix(1.0f, 1.0f, 0.1f, 100.0f);

	// Create a scene node for the camera
	std::shared_ptr<SceneCamera> sceneCamera = std::make_shared<SceneCamera>("main camera", camera);

	// Add the camera node to the scene
	m_scene.AddSceneNode(sceneCamera);

	// Set the camera scene node to be controlled by the camera controller
	m_cameraController.SetCamera(sceneCamera);
}

void PostFXSceneViewerApplication::InitializeLights()
{
	// Create a directional light and add it to the scene
	std::shared_ptr<DirectionalLight> directionalLight = std::make_shared<DirectionalLight>();
	directionalLight->SetDirection(glm::vec3(0.0f, -1.0f, -0.3f)); // It will be normalized inside the function
	directionalLight->SetIntensity(3.0f);
	m_scene.AddSceneNode(std::make_shared<SceneLight>("directional light", directionalLight));

	// Create a point light and add it to the scene
	//std::shared_ptr<PointLight> pointLight = std::make_shared<PointLight>();
	//pointLight->SetPosition(glm::vec3(0, 0, 0));
	//pointLight->SetDistanceAttenuation(glm::vec2(5.0f, 10.0f));
	//m_scene.AddSceneNode(std::make_shared<SceneLight>("point light", pointLight));
}

void PostFXSceneViewerApplication::InitializeMaterials()
{
	// G-buffer material
	{
		// Load and build shader
		std::vector<const char*> vertexShaderPaths;
		vertexShaderPaths.push_back("shaders/version330.glsl");
		vertexShaderPaths.push_back("shaders/default.vert");
		Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

		std::vector<const char*> fragmentShaderPaths;
		fragmentShaderPaths.push_back("shaders/version330.glsl");
		fragmentShaderPaths.push_back("shaders/utils.glsl");
		fragmentShaderPaths.push_back("shaders/default.frag");
		Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

		std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
		shaderProgramPtr->Build(vertexShader, fragmentShader);

		// Get transform related uniform locations
		ShaderProgram::Location worldViewMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldViewMatrix");
		ShaderProgram::Location worldViewProjMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldViewProjMatrix");

		// Register shader with renderer
		m_renderer.RegisterShaderProgram(shaderProgramPtr,
			[=](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
			{
				shaderProgram.SetUniform(worldViewMatrixLocation, camera.GetViewMatrix() * worldMatrix);
				shaderProgram.SetUniform(worldViewProjMatrixLocation, camera.GetViewProjectionMatrix() * worldMatrix);
			},
			nullptr
		);

		// Filter out uniforms that are not material properties
		ShaderUniformCollection::NameSet filteredUniforms;
		filteredUniforms.insert("WorldViewMatrix");
		filteredUniforms.insert("WorldViewProjMatrix");

		// Create material
		m_gbufferMaterial = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);
		m_gbufferMaterial->SetUniformValue("Color", glm::vec3(1.0f));
	}

	// Deferred material
	{
		std::vector<const char*> vertexShaderPaths;
		vertexShaderPaths.push_back("shaders/version330.glsl");
		vertexShaderPaths.push_back("shaders/renderer/deferred.vert");
		Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

		std::vector<const char*> fragmentShaderPaths;
		fragmentShaderPaths.push_back("shaders/version330.glsl");
		fragmentShaderPaths.push_back("shaders/utils.glsl");
		fragmentShaderPaths.push_back("shaders/lambert-ggx.glsl");
		fragmentShaderPaths.push_back("shaders/lighting.glsl");
		fragmentShaderPaths.push_back("shaders/renderer/deferred.frag");
		Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

		std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
		shaderProgramPtr->Build(vertexShader, fragmentShader);

		// Filter out uniforms that are not material properties
		ShaderUniformCollection::NameSet filteredUniforms;
		filteredUniforms.insert("InvViewMatrix");
		filteredUniforms.insert("InvProjMatrix");
		filteredUniforms.insert("WorldViewProjMatrix");
		filteredUniforms.insert("LightIndirect");
		filteredUniforms.insert("LightColor");
		filteredUniforms.insert("LightPosition");
		filteredUniforms.insert("LightDirection");
		filteredUniforms.insert("LightAttenuation");

		// Get transform related uniform locations
		ShaderProgram::Location invViewMatrixLocation = shaderProgramPtr->GetUniformLocation("InvViewMatrix");
		ShaderProgram::Location invProjMatrixLocation = shaderProgramPtr->GetUniformLocation("InvProjMatrix");
		ShaderProgram::Location worldViewProjMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldViewProjMatrix");

		// Register shader with renderer
		m_renderer.RegisterShaderProgram(shaderProgramPtr,
			[=](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
			{
				if (cameraChanged)
				{
					shaderProgram.SetUniform(invViewMatrixLocation, glm::inverse(camera.GetViewMatrix()));
					shaderProgram.SetUniform(invProjMatrixLocation, glm::inverse(camera.GetProjectionMatrix()));
				}
				shaderProgram.SetUniform(worldViewProjMatrixLocation, camera.GetViewProjectionMatrix() * worldMatrix);
			},
			m_renderer.GetDefaultUpdateLightsFunction(*shaderProgramPtr)
		);

		// Create material
		m_deferredMaterial = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);
	}

	// Forward material
	{
		std::vector<const char*> vertexShaderPaths;
		vertexShaderPaths.push_back("shaders/version330.glsl");
		vertexShaderPaths.push_back("shaders/renderer/forward.vert");
		Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

		std::vector<const char*> fragmentShaderPaths;
		fragmentShaderPaths.push_back("shaders/version330.glsl");
		fragmentShaderPaths.push_back("shaders/utils.glsl");
		fragmentShaderPaths.push_back("shaders/lambert-ggx.glsl");
		fragmentShaderPaths.push_back("shaders/lighting.glsl");
		fragmentShaderPaths.push_back("shaders/renderer/ice.frag");
		Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

		std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
		shaderProgramPtr->Build(vertexShader, fragmentShader);

		// Get transform related uniform locations
		ShaderProgram::Location cameraPositionLocation = shaderProgramPtr->GetUniformLocation("CameraPosition");
		ShaderProgram::Location worldMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldMatrix");
		ShaderProgram::Location viewProjMatrixLocation = shaderProgramPtr->GetUniformLocation("ViewProjMatrix");
		ShaderProgram::Location timeLocation = shaderProgramPtr->GetUniformLocation("Time");
		ShaderProgram::Location iceEpicenterLocation = shaderProgramPtr->GetUniformLocation("IceEpiCenter");
		ShaderProgram::Location debugModeLocation = shaderProgramPtr->GetUniformLocation("DebugMode");

		ShaderProgram::Location iceSamplingScaleLocation = shaderProgramPtr->GetUniformLocation("IceSamplingScale");

		// Register shader with renderer
		m_renderer.RegisterShaderProgram(shaderProgramPtr,
			[=](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
			{
				if (cameraChanged)
				{
					shaderProgram.SetUniform(cameraPositionLocation, camera.ExtractTranslation());
					shaderProgram.SetUniform(viewProjMatrixLocation, camera.GetViewProjectionMatrix());
				}
				shaderProgram.SetUniform(worldMatrixLocation, worldMatrix);
				shaderProgram.SetUniform(timeLocation, m_time);
				shaderProgram.SetUniform(iceEpicenterLocation, m_iceEpicenter);
				shaderProgram.SetUniform(debugModeLocation, m_debugMode ? 1 : 0);
				shaderProgram.SetUniform(iceSamplingScaleLocation, m_iceSamplingScale);
			},
			m_renderer.GetDefaultUpdateLightsFunction(*shaderProgramPtr)
		);

		// Filter out uniforms that are not material properties
		ShaderUniformCollection::NameSet filteredUniforms;
		filteredUniforms.insert("CameraPosition");
		filteredUniforms.insert("WorldMatrix");
		filteredUniforms.insert("ViewProjMatrix");
		filteredUniforms.insert("LightIndirect");
		filteredUniforms.insert("LightColor");
		filteredUniforms.insert("LightPosition");
		filteredUniforms.insert("LightDirection");
		filteredUniforms.insert("LightAttenuation");

		// Create material
		m_forwardMaterial = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);
		m_forwardMaterial->SetBlendEquation(Material::BlendEquation::Add);
		m_forwardMaterial->SetBlendParams(Material::BlendParam::SourceAlpha, Material::BlendParam::OneMinusSourceAlpha);
		m_forwardMaterial->SetDepthWrite(false);

		m_frozenMaterial = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);
		m_frozenMaterial->SetBlendEquation(Material::BlendEquation::Add);
		m_frozenMaterial->SetBlendParams(Material::BlendParam::SourceAlpha, Material::BlendParam::OneMinusSourceAlpha);
		m_frozenMaterial->SetDepthWrite(false);
	}
}

void PostFXSceneViewerApplication::InitializeModels()
{
	//m_skyboxTexture = TextureCubemapLoader::LoadTextureShared("models/skybox/yoga_studio.hdr", TextureObject::FormatRGB, TextureObject::InternalFormatRGB16F);
	m_skyboxTexture = TextureCubemapLoader::LoadTextureShared("models/skybox/lonely_road_afternoon_puresky_1k.png", TextureObject::FormatRGB, TextureObject::InternalFormatRGB16F);

	m_frostTexture = Texture2DLoader::LoadTextureShared("models/ice/ground_0031_color_1k.jpg", TextureObject::FormatRGB, TextureObject::InternalFormatRGB16F);

	m_frostCombinedTexture = Texture2DLoader::LoadTextureShared("models/ice/ground_0031_ao_rough_sub.png", TextureObject::FormatRGB, TextureObject::InternalFormatRGB16F);

	m_perlinNoiseTexture = CreateHeightMap(1024, 1024, glm::ivec2(0, 0));

	m_vornoiNoiseTexture = Texture2DLoader::LoadTextureShared("models/ice/ColorfulCells.png", TextureObject::FormatRGB, TextureObject::InternalFormatRGB16F);


	m_skyboxTexture->Bind();
	float maxLod;
	m_skyboxTexture->GetParameter(TextureObject::ParameterFloat::MaxLod, maxLod);
	TextureCubemapObject::Unbind();

	// Set the environment texture on the deferred material
	m_deferredMaterial->SetUniformValue("EnvironmentTexture", m_skyboxTexture);
	m_deferredMaterial->SetUniformValue("EnvironmentMaxLod", maxLod);

	m_forwardMaterial->SetUniformValue("EnvironmentTexture", m_skyboxTexture);
	m_forwardMaterial->SetUniformValue("EnvironmentMaxLod", maxLod);

	m_frozenMaterial->SetUniformValue("EnvironmentTexture", m_skyboxTexture);
	m_frozenMaterial->SetUniformValue("EnvironmentMaxLod", maxLod);

	m_frozenMaterial->SetUniformValue("IceColorTexture", m_frostTexture);
	m_frozenMaterial->SetUniformValue("IceCombinedTexture", m_frostCombinedTexture);

	m_frozenMaterial->SetUniformValue("IceEpicenter", m_iceEpicenter);
	m_frozenMaterial->SetUniformValue("DebugMode", m_debugMode ? 1 : 0);

	m_frozenMaterial->SetUniformValue("IceSamplingScale", m_iceSamplingScale);


	m_frozenMaterial->SetUniformValue("PerlinNoiseTexture", m_perlinNoiseTexture);
	m_frozenMaterial->SetUniformValue("VoronoiNoiseTexture", m_vornoiNoiseTexture);


	// Configure loader
	ModelLoader loader(m_gbufferMaterial);

	// Create a new material copy for each submaterial
	loader.SetCreateMaterials(true);

	// Flip vertically textures loaded by the model loader
	loader.GetTexture2DLoader().SetFlipVertical(true);

	// Link vertex properties to attributes
	loader.SetMaterialAttribute(VertexAttribute::Semantic::Position, "VertexPosition");
	loader.SetMaterialAttribute(VertexAttribute::Semantic::Normal, "VertexNormal");
	loader.SetMaterialAttribute(VertexAttribute::Semantic::Tangent, "VertexTangent");
	loader.SetMaterialAttribute(VertexAttribute::Semantic::Bitangent, "VertexBitangent");
	loader.SetMaterialAttribute(VertexAttribute::Semantic::TexCoord0, "VertexTexCoord");

	// Link material properties to uniforms
	loader.SetMaterialProperty(ModelLoader::MaterialProperty::DiffuseColor, "Color");
	loader.SetMaterialProperty(ModelLoader::MaterialProperty::DiffuseTexture, "ColorTexture");
	loader.SetMaterialProperty(ModelLoader::MaterialProperty::NormalTexture, "NormalTexture");
	loader.SetMaterialProperty(ModelLoader::MaterialProperty::SpecularTexture, "SpecularTexture");

	// Load models
	/*{
		std::shared_ptr<Model> cannonModel = loader.LoadShared("models/cannon/cannon.obj");
		std::shared_ptr<SceneModel> sceneModel = std::make_shared<SceneModel>("cannon", cannonModel);
		sceneModel->GetTransform()->SetTranslation(glm::vec3(-3.f, 0.01f, -3.f));
		sceneModel->GetTransform()->SetScale(glm::vec3(0.99f));
		m_scene.AddSceneNode(sceneModel);
	}*/

	// Configure loader
	//ModelLoader forwardLoader(m_forwardMaterial);
	ModelLoader forwardLoader(m_frozenMaterial);


	// Create a new material copy for each submaterial
	forwardLoader.SetCreateMaterials(true);

	// Flip vertically textures loaded by the model loader
	forwardLoader.GetTexture2DLoader().SetFlipVertical(true);

	// Link vertex properties to attributes
	forwardLoader.SetMaterialAttribute(VertexAttribute::Semantic::Position, "VertexPosition");
	forwardLoader.SetMaterialAttribute(VertexAttribute::Semantic::Normal, "VertexNormal");
	forwardLoader.SetMaterialAttribute(VertexAttribute::Semantic::Tangent, "VertexTangent");
	forwardLoader.SetMaterialAttribute(VertexAttribute::Semantic::Bitangent, "VertexBitangent");
	forwardLoader.SetMaterialAttribute(VertexAttribute::Semantic::TexCoord0, "VertexTexCoord");

	// Link material properties to uniforms
	forwardLoader.SetMaterialProperty(ModelLoader::MaterialProperty::DiffuseColor, "Color");
	forwardLoader.SetMaterialProperty(ModelLoader::MaterialProperty::DiffuseTexture, "ColorTexture");
	forwardLoader.SetMaterialProperty(ModelLoader::MaterialProperty::NormalTexture, "NormalTexture");
	forwardLoader.SetMaterialProperty(ModelLoader::MaterialProperty::SpecularTexture, "SpecularTexture");

	int forwardIndex = 0;
	glm::vec2 sphereDistance(3.0f, 3.0f);
	std::shared_ptr<Model> sphereModel = forwardLoader.LoadShared("models/sphere/sphere.obj");

	std::shared_ptr<Model> forwardCannonModel = forwardLoader.LoadShared("models/cannon/cannon.obj");
	std::shared_ptr<Model> chestModel = forwardLoader.LoadShared("models/treasure_chest/treasure_chest.obj");
	std::shared_ptr<Model> alarmModel = forwardLoader.LoadShared("models/alarm_clock/alarm_clock.obj");
	std::shared_ptr<Model> catModel = forwardLoader.LoadShared("models/cat/concrete_cat_statue_1k.obj");

	//std::shared_ptr<Model> libertyModel = forwardLoader.LoadShared("models/LibertyStatue/LibertyStatue.obj");

	{
		//DEBUGGING TOOL
		auto center = glm::vec2(0);
		std::string name("epicenterVisual ");
		name += std::to_string(forwardIndex++);
		epicenterModel = std::make_shared<SceneModel>(name, sphereModel);
		epicenterModel->GetTransform()->SetTranslation(glm::vec3(center.x * sphereDistance.x, 0.0f, center.y * sphereDistance.y));
		m_scene.AddSceneNode(epicenterModel);
	}

	auto center = glm::vec2(1);
	{
		std::string name("forwardModel ");
		name += std::to_string(forwardIndex++);
		std::shared_ptr<SceneModel> sceneModel = std::make_shared<SceneModel>(name, sphereModel);
		sceneModel->GetTransform()->SetTranslation(glm::vec3(center.x * sphereDistance.x, 0.0f, center.y * sphereDistance.y));
		m_scene.AddSceneNode(sceneModel);
	}

	center = glm::vec2(-1);
	{
		std::string name("forwardModel ");
		name += std::to_string(forwardIndex++);
		std::shared_ptr<SceneModel> sceneModel = std::make_shared<SceneModel>(name, forwardCannonModel);
		sceneModel->GetTransform()->SetTranslation(glm::vec3(center.x * sphereDistance.x, 0.0f, center.y * sphereDistance.y));
		m_scene.AddSceneNode(sceneModel);
	}

	center = glm::vec2(-1, 0);
	{
		std::string name("forwardModel ");
		name += std::to_string(forwardIndex++);
		std::shared_ptr<SceneModel> sceneModel = std::make_shared<SceneModel>(name, chestModel);
		sceneModel->GetTransform()->SetTranslation(glm::vec3(center.x * sphereDistance.x, 0.0f, center.y * sphereDistance.y));
		m_scene.AddSceneNode(sceneModel);
	}

	center = glm::vec2(0, -1);
	{
		std::string name("forwardModel ");
		name += std::to_string(forwardIndex++);
		std::shared_ptr<SceneModel> sceneModel = std::make_shared<SceneModel>(name, catModel);
		sceneModel->GetTransform()->SetTranslation(glm::vec3(center.x * sphereDistance.x, 0.0f, center.y * sphereDistance.y));
		sceneModel->GetTransform()->SetScale(glm::vec3(4, 4, 4));

		m_scene.AddSceneNode(sceneModel);
	}

	center = glm::vec2(1, 0);
	{
		std::string name("forwardModel ");
		name += std::to_string(forwardIndex++);
		std::shared_ptr<SceneModel> sceneModel = std::make_shared<SceneModel>(name, sphereModel);
		sceneModel->GetTransform()->SetTranslation(glm::vec3(center.x * sphereDistance.x, 0.0f, center.y * sphereDistance.y));
		m_scene.AddSceneNode(sceneModel);
	}




	/*for (int j = 1; j <= 1; ++j)
	{
		for (int i = 0; i <= 1; ++i)
		{

			std::string name("forwardModel ");
			name += std::to_string(forwardIndex++);
			std::shared_ptr<SceneModel> sceneModel = std::make_shared<SceneModel>(name, sphereModel);
			sceneModel->GetTransform()->SetTranslation(glm::vec3(i * sphereDistance.x, 0.0f, j * sphereDistance.y));
			m_scene.AddSceneNode(sceneModel);
		}
	}*/
}

void PostFXSceneViewerApplication::InitializeFramebuffers()
{
	int width, height;
	GetMainWindow().GetDimensions(width, height);

	// Scene Texture
	m_sceneTexture = std::make_shared<Texture2DObject>();
	m_sceneTexture->Bind();
	m_sceneTexture->SetImage(0, width, height, TextureObject::FormatRGBA, TextureObject::InternalFormat::InternalFormatRGBA16F);
	m_sceneTexture->SetParameter(TextureObject::ParameterEnum::MinFilter, GL_LINEAR);
	m_sceneTexture->SetParameter(TextureObject::ParameterEnum::MagFilter, GL_LINEAR);
	Texture2DObject::Unbind();

	// Scene framebuffer
	m_sceneFramebuffer->Bind();
	m_sceneFramebuffer->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Depth, *m_depthTexture);
	m_sceneFramebuffer->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Color0, *m_sceneTexture);
	m_sceneFramebuffer->SetDrawBuffers(std::array<FramebufferObject::Attachment, 1>({ FramebufferObject::Attachment::Color0 }));
	FramebufferObject::Unbind();

	// Add temp textures and frame buffers
	for (int i = 0; i < m_tempFramebuffers.size(); ++i)
	{
		m_tempTextures[i] = std::make_shared<Texture2DObject>();
		m_tempTextures[i]->Bind();
		m_tempTextures[i]->SetImage(0, width, height, TextureObject::FormatRGBA, TextureObject::InternalFormat::InternalFormatRGBA16F);
		m_tempTextures[i]->SetParameter(TextureObject::ParameterEnum::WrapS, GL_CLAMP_TO_EDGE);
		m_tempTextures[i]->SetParameter(TextureObject::ParameterEnum::WrapT, GL_CLAMP_TO_EDGE);
		m_tempTextures[i]->SetParameter(TextureObject::ParameterEnum::MinFilter, GL_LINEAR);
		m_tempTextures[i]->SetParameter(TextureObject::ParameterEnum::MagFilter, GL_LINEAR);

		m_tempFramebuffers[i] = std::make_shared<FramebufferObject>();
		m_tempFramebuffers[i]->Bind();
		m_tempFramebuffers[i]->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Color0, *m_tempTextures[i]);
		m_tempFramebuffers[i]->SetDrawBuffers(std::array<FramebufferObject::Attachment, 1>({ FramebufferObject::Attachment::Color0 }));
	}
	Texture2DObject::Unbind();
	FramebufferObject::Unbind();
}

void PostFXSceneViewerApplication::InitializeRenderer()
{
	int width, height;
	GetMainWindow().GetDimensions(width, height);

	//Default collection is opaque only
	m_renderer.SetDrawcallCollectionSupportedFunction(0, IsOpaque);

	// Add another collection for transparent objects
	m_transparentCollection = m_renderer.AddDrawcallCollection(IsTransparent);

	// Set up deferred passes
	{
		std::unique_ptr<GBufferRenderPass> gbufferRenderPass(std::make_unique<GBufferRenderPass>(width, height));

		// Set the g-buffer textures as properties of the deferred material
		m_deferredMaterial->SetUniformValue("DepthTexture", gbufferRenderPass->GetDepthTexture());
		m_deferredMaterial->SetUniformValue("AlbedoTexture", gbufferRenderPass->GetAlbedoTexture());
		m_deferredMaterial->SetUniformValue("NormalTexture", gbufferRenderPass->GetNormalTexture());
		m_deferredMaterial->SetUniformValue("OthersTexture", gbufferRenderPass->GetOthersTexture());

		// Get the depth texture from the gbuffer pass - This could be reworked
		m_depthTexture = gbufferRenderPass->GetDepthTexture();

		// Add the render passes
		m_renderer.AddRenderPass(std::move(gbufferRenderPass));
		m_renderer.AddRenderPass(std::make_unique<DeferredRenderPass>(m_deferredMaterial, m_sceneFramebuffer));
	}

	// Initialize the framebuffers and the textures they use
	InitializeFramebuffers();

	// Skybox pass
	m_renderer.AddRenderPass(std::make_unique<SkyboxRenderPass>(m_skyboxTexture));



	//render ice
	//m_frozenMaterial->SetDepthTestFunction(Material::TestFunction::Equal);

	//std::unique_ptr<GBufferRenderPass> gbufferRenderPass(std::make_unique<GBufferRenderPass>(width, height));

	//// Set the g-buffer textures as properties of the deferred material
	//m_frozenMaterial->SetUniformValue("DepthTexture", gbufferRenderPass->GetDepthTexture());
	//m_frozenMaterial->SetUniformValue("AlbedoTexture", gbufferRenderPass->GetAlbedoTexture());
	//m_frozenMaterial->SetUniformValue("NormalTexture", gbufferRenderPass->GetNormalTexture());
	//m_frozenMaterial->SetUniformValue("OthersTexture", gbufferRenderPass->GetOthersTexture());
	//m_frozenMaterial->
	//m_frozenMaterial->
	//m_renderer.AddRenderPass(std::make_unique<DeferredRenderPass>(m_frozenMaterial, m_sceneFramebuffer));

	m_renderer.AddRenderPass(std::make_unique<ForwardRenderPass>(m_transparentCollection));

	// Create a copy pass from m_sceneTexture to the first temporary texture
	std::shared_ptr<Material> copyMaterial = CreatePostFXMaterial("shaders/postfx/copy.frag", m_sceneTexture);
	m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(copyMaterial, m_tempFramebuffers[0]));

	// Replace the copy pass with a new bloom pass
	m_bloomMaterial = CreatePostFXMaterial("shaders/postfx/bloom.frag", m_sceneTexture);
	m_bloomMaterial->SetUniformValue("Range", glm::vec2(2.0f, 3.0f));
	m_bloomMaterial->SetUniformValue("Intensity", 1.0f);
	m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(m_bloomMaterial, m_tempFramebuffers[0]));

	// Add blur passes
	std::shared_ptr<Material> blurHorizontalMaterial = CreatePostFXMaterial("shaders/postfx/blur.frag", m_tempTextures[0]);
	blurHorizontalMaterial->SetUniformValue("Scale", glm::vec2(1.0f / width, 0.0f));
	std::shared_ptr<Material> blurVerticalMaterial = CreatePostFXMaterial("shaders/postfx/blur.frag", m_tempTextures[1]);
	blurVerticalMaterial->SetUniformValue("Scale", glm::vec2(0.0f, 1.0f / height));
	for (int i = 0; i < m_blurIterations; ++i)
	{
		m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(blurHorizontalMaterial, m_tempFramebuffers[1]));
		m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(blurVerticalMaterial, m_tempFramebuffers[0]));
	}

	// Final pass
	m_composeMaterial = CreatePostFXMaterial("shaders/postfx/compose.frag", m_sceneTexture);

	// Set exposure uniform default value
	m_composeMaterial->SetUniformValue("Exposure", m_exposure);

	// Set uniform default values
	m_composeMaterial->SetUniformValue("Contrast", m_contrast);
	m_composeMaterial->SetUniformValue("HueShift", m_hueShift);
	m_composeMaterial->SetUniformValue("Saturation", m_saturation);
	m_composeMaterial->SetUniformValue("ColorFilter", m_colorFilter);

	// Set the bloom texture uniform
	m_composeMaterial->SetUniformValue("BloomTexture", m_tempTextures[0]);

	m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(m_composeMaterial, m_renderer.GetDefaultFramebuffer()));
}

std::shared_ptr<Material> PostFXSceneViewerApplication::CreatePostFXMaterial(const char* fragmentShaderPath, std::shared_ptr<Texture2DObject> sourceTexture)
{
	// We could keep this vertex shader and reuse it, but it looks simpler this way
	std::vector<const char*> vertexShaderPaths;
	vertexShaderPaths.push_back("shaders/version330.glsl");
	vertexShaderPaths.push_back("shaders/renderer/fullscreen.vert");
	Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

	std::vector<const char*> fragmentShaderPaths;
	fragmentShaderPaths.push_back("shaders/version330.glsl");
	fragmentShaderPaths.push_back("shaders/utils.glsl");
	fragmentShaderPaths.push_back(fragmentShaderPath);
	Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

	std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
	shaderProgramPtr->Build(vertexShader, fragmentShader);

	// Create material
	std::shared_ptr<Material> material = std::make_shared<Material>(shaderProgramPtr);
	material->SetUniformValue("SourceTexture", sourceTexture);

	return material;
}

Renderer::UpdateTransformsFunction PostFXSceneViewerApplication::GetFullscreenTransformFunction(std::shared_ptr<ShaderProgram> shaderProgramPtr) const
{
	// Get transform related uniform locations
	ShaderProgram::Location invViewMatrixLocation = shaderProgramPtr->GetUniformLocation("InvViewMatrix");
	ShaderProgram::Location invProjMatrixLocation = shaderProgramPtr->GetUniformLocation("InvProjMatrix");
	ShaderProgram::Location worldViewProjMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldViewProjMatrix");

	// Return transform function
	return [=](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
		{
			if (cameraChanged)
			{
				shaderProgram.SetUniform(invViewMatrixLocation, glm::inverse(camera.GetViewMatrix()));
				shaderProgram.SetUniform(invProjMatrixLocation, glm::inverse(camera.GetProjectionMatrix()));
			}
			shaderProgram.SetUniform(worldViewProjMatrixLocation, camera.GetViewProjectionMatrix() * worldMatrix);

		};
}

void PostFXSceneViewerApplication::RenderGUI()
{
	m_imGui.BeginFrame();

	// Draw GUI for scene nodes, using the visitor pattern
	ImGuiSceneVisitor imGuiVisitor(m_imGui, "Scene");
	m_scene.AcceptVisitor(imGuiVisitor);

	// Draw GUI for camera controller
	m_cameraController.DrawGUI(m_imGui);



	if (auto window = m_imGui.UseWindow("Post FX"))
	{
		if (m_frozenMaterial)
		{
			if (ImGui::SliderFloat("Time", &m_time, 0.f, 1000.0f))
			{
				m_frozenMaterial->SetUniformValue("Time", m_time);
			}
			if (ImGui::DragFloat("TimeScale", &m_timeScale, 0.1f, -10.f, 10.0f))
			{
				//m_frozenMaterial->SetUniformValue("Time", m_timeScale);
			}
			if (ImGui::DragFloat3("Ice Epicenter", &m_iceEpicenter[0], 0.1f))
			{
				m_frozenMaterial->SetUniformValue("IceEpicenter", m_iceEpicenter);
				//m_composeMaterial->SetUniformValue("IceEpicenter", m_iceEpicenter);
			}
			if (ImGui::InputFloat2("Ice Sampling Scale", &m_iceSamplingScale[0]))
			{
				m_frozenMaterial->SetUniformValue("IceSamplingScale", m_iceSamplingScale);

			}

			if (ImGui::Checkbox("DebugMode", &m_debugMode))
			{
				m_frozenMaterial->SetUniformValue("DebugMode", m_debugMode ? 1 : 0);
			}
		}
		if (m_composeMaterial)
		{
			if (ImGui::DragFloat("Exposure", &m_exposure, 0.01f, 0.01f, 5.0f))
			{
				m_composeMaterial->SetUniformValue("Exposure", m_exposure);
			}

			ImGui::Separator();

			if (ImGui::SliderFloat("Contrast", &m_contrast, 0.5f, 1.5f))
			{
				m_composeMaterial->SetUniformValue("Contrast", m_contrast);
			}
			if (ImGui::SliderFloat("Hue Shift", &m_hueShift, -0.5f, 0.5f))
			{
				m_composeMaterial->SetUniformValue("HueShift", m_hueShift);
			}
			if (ImGui::SliderFloat("Saturation", &m_saturation, 0.0f, 2.0f))
			{
				m_composeMaterial->SetUniformValue("Saturation", m_saturation);
			}
			if (ImGui::ColorEdit3("Color Filter", &m_colorFilter[0]))
			{
				m_composeMaterial->SetUniformValue("ColorFilter", m_colorFilter);
			}

			ImGui::Separator();

			if (ImGui::DragFloat2("Bloom Range", &m_bloomRange[0], 0.1f, 0.1f, 10.0f))
			{
				m_bloomMaterial->SetUniformValue("Range", m_bloomRange);
			}
			if (ImGui::DragFloat("Bloom Intensity", &m_bloomIntensity, 0.1f, 0.0f, 5.0f))
			{
				m_bloomMaterial->SetUniformValue("Intensity", m_bloomIntensity);
			}
		}
	}

	m_imGui.EndFrame();
}

bool PostFXSceneViewerApplication::IsOpaque(const Renderer::DrawcallInfo& drawcallInfo)
{
	return !IsTransparent(drawcallInfo);
}

bool PostFXSceneViewerApplication::IsTransparent(const Renderer::DrawcallInfo& drawcallInfo)
{
	return drawcallInfo.GetMaterial().HasBlend();
}


std::shared_ptr<Texture2DObject> PostFXSceneViewerApplication::CreateHeightMap(unsigned int width, unsigned int height, glm::ivec2 coords)
{
	std::shared_ptr<Texture2DObject> heightmap = std::make_shared<Texture2DObject>();

	std::vector<float> pixels(height * width);
	for (unsigned int j = 0; j < height; ++j)
	{
		for (unsigned int i = 0; i < width; ++i)
		{
			float x = static_cast<float>(i) / (width - 1) + coords.x;
			float y = static_cast<float>(j) / (height - 1) + coords.y;
			pixels[j * width + i] = stb_perlin_fbm_noise3(x, y, 0.0f, 1.9f, 0.5f, 8) * 0.5f;
		}
	}

	heightmap->Bind();
	heightmap->SetImage<float>(0, width, height, TextureObject::FormatR, TextureObject::InternalFormatR16F, pixels);
	heightmap->GenerateMipmap();

	return heightmap;
}
