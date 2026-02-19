#pragma once

// batteries
#include "batteries/scene.h"
#include "batteries/lights.h"


// ew
#include "ew/model.h"
#include "ew/shader.h"
#include "ew/texture.h"
#include "ew/mesh.h"

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
    std::unique_ptr<ew::Shader> water;

    // mipmaps
    std::unique_ptr<ew::Texture> wave_spec; // mipmap: 0
    std::unique_ptr<ew::Texture> wave_tex;  // mipmap: 1
    std::unique_ptr<ew::Texture> wave_warp;  // mipmap: 2

    //water plane
    ew::Mesh plane;
};
