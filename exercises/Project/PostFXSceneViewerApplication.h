#pragma once

#include <ituGL/application/Application.h>

#include <ituGL/scene/Scene.h>
#include <ituGL/texture/FramebufferObject.h>
#include <ituGL/renderer/Renderer.h>
#include <ituGL/camera/CameraController.h>
#include <ituGL/utils/DearImGui.h>
#include <array>
#include <ituGL/scene/SceneModel.h>

class Texture2DObject;
class TextureCubemapObject;
class Material;

class PostFXSceneViewerApplication : public Application
{
public:
    PostFXSceneViewerApplication();

protected:
    void Initialize() override;
    void Update() override;
    void Render() override;
    void Cleanup() override;

private:
    void InitializeCamera();
    void InitializeLights();
    void InitializeMaterials();
    void InitializeModels();
    void InitializeFramebuffers();
    void InitializeRenderer();

    std::shared_ptr<Material> CreatePostFXMaterial(const char* fragmentShaderPath, std::shared_ptr<Texture2DObject> sourceTexture = nullptr);

    Renderer::UpdateTransformsFunction GetFullscreenTransformFunction(std::shared_ptr<ShaderProgram> shaderProgramPtr) const;

    void RenderGUI();

    static bool IsOpaque(const Renderer::DrawcallInfo& drawcallInfo);
    static bool IsTransparent(const Renderer::DrawcallInfo& drawcallInfo);

    std::shared_ptr<Texture2DObject> CreateHeightMap(unsigned int width, unsigned int height, glm::ivec2 coords);

private:
    // Helper object for debug GUI
    DearImGui m_imGui;

    // Camera controller
    CameraController m_cameraController;

    // Global scene
    Scene m_scene;

    // Renderer
    Renderer m_renderer;

    // Skybox texture
    std::shared_ptr<TextureCubemapObject> m_skyboxTexture;

    // Materials
    std::shared_ptr<Material> m_gbufferMaterial;
    std::shared_ptr<Material> m_deferredMaterial;
    std::shared_ptr<Material> m_forwardMaterial;
    std::shared_ptr<Material> m_composeMaterial;
    std::shared_ptr<Material> m_bloomMaterial;

    std::shared_ptr<Material> m_frozenMaterial;

    std::shared_ptr<Texture2DObject> m_frostCombinedTexture;
    std::shared_ptr<Texture2DObject> m_frostTexture;

    // Framebuffers
    std::shared_ptr<FramebufferObject> m_sceneFramebuffer;
    std::shared_ptr<Texture2DObject> m_depthTexture;
    std::shared_ptr<Texture2DObject> m_sceneTexture;
    std::array<std::shared_ptr<FramebufferObject>, 2> m_tempFramebuffers;
    std::array<std::shared_ptr<Texture2DObject>, 2> m_tempTextures;
    std::shared_ptr<Texture2DObject> m_perlinNoiseTexture;
    std::shared_ptr<Texture2DObject> m_vornoiNoiseTexture;


    //Epicenter sphere Model
    std::shared_ptr<SceneModel> epicenterModel;

    // Configuration values
    float m_exposure;
    float m_contrast;
    float m_hueShift;
    float m_saturation;
    glm::vec3 m_colorFilter;
    int m_blurIterations;
    glm::vec2 m_bloomRange;
    float m_bloomIntensity;
    float m_time;
    glm::vec3 m_iceEpicenter;
    bool m_debugMode;
    glm::vec2 m_iceSamplingScale;
    float m_timeScale;

    // Drawcall collections
    unsigned int m_transparentCollection;
};
