#pragma once

// batteries
#include "batteries/scene.h"
#include "batteries/lights.h"
#include "batteries/opengl.h"

// ew
#include "ew/model.h"
#include "ew/shader.h"
#include "ew/texture.h"
#include "ew/camera.h"
#include "ew/procGen.h"

class Scene final : public batteries::Scene
{
  public:
    Scene();
    virtual ~Scene();

    void Update(float dt);
    void Render(void);
    void Debug(void);

  private:
    std::unique_ptr<ew::Model> suzanne;
    std::unique_ptr<ew::Shader> toon;
    std::unique_ptr<ew::Shader> shadow;
    std::unique_ptr<ew::Texture> txGradient;

    // post-processing effects
    std::unique_ptr<ew::Shader> postprocess;

    ew::Mesh planeMesh;

    batteries::light_t light;

    struct {
      glm::vec3 color1;
      glm::vec3 color2;
    } palette;

    GLuint shadowFBO;
    GLuint shadowMap;

    ew::Camera lightCamera;
    glm::mat4 lightSpaceMatrix;

    GLuint fbo;
    GLuint fboTexture;
    GLuint fboDepth;
};
