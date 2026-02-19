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
    std::unique_ptr<ew::Texture> txWater128; // mipmap: 0
    std::unique_ptr<ew::Texture> txWater64;  // mipmap: 1
    std::unique_ptr<ew::Texture> txWater32;  // mipmap: 2
    std::unique_ptr<ew::Texture> txWater16;  // mipmap: 3
    std::unique_ptr<ew::Texture> txWater8;   // mipmap: 4

    //water plane
    ew::Mesh plane;
};
