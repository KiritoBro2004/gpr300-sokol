#include "scene.h"

// imgui
#include "imgui/imgui.h"
#include "imguizmo/imguizmo.h"

// glm
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

// batteries
#include "batteries/opengl.h"

#include "ew/procGen.h"


struct {
    glm::vec3 water_color = glm::vec3(0, 0, 0);//glm::vec3(0.5, 0, 0.5);
} debug;
Scene::Scene()
{
    suzanne = std::make_unique<ew::Model>("assets/models/suzanne.obj");
    water = std::make_unique<ew::Shader>(
        "assets/shaders/windwaker/water.vs",
        "assets/shaders/windwaker/water.fs"
    );

    txWater128 = std::make_unique<ew::Texture>("assets/textures/windwaker/water128.png");
    txWater64 = std::make_unique<ew::Texture>("assets/textures/windwaker/water64.png");
    txWater32 = std::make_unique<ew::Texture>("assets/textures/windwaker/water32.png");
    txWater16 = std::make_unique<ew::Texture>("assets/textures/windwaker/water16.png");
    txWater8 = std::make_unique<ew::Texture>("assets/textures/windwaker/water8.png");

    //draw water okalne
    plane.load(ew::createPlane(100.0f, 100.0f, 10));
}

Scene::~Scene()
{
}

void Scene::Update(float dt)
{
    batteries::Scene::Update(dt);

    /* body */
}

glm::vec3 light_color = glm::vec3(1.0f);
void Scene::Render(void)
{
    const auto view_proj = camera.Projection() * camera.View();

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    
    auto index = 0;
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D, txWater128->getID());

    water->use();

    water->setInt("texture0", index);

    water->setFloat("time", time.absolute);


    // scene matrices
    water->setMat4("model", glm::mat4(1.0f));
    water->setMat4("view_proj", view_proj);

    water->setVec3("camera", camera.position);
    water->setVec3("water_color", debug.water_color);

    // draw suzanne
    plane.draw();
}

void Scene::Debug(void)
{
    ImGuizmo::BeginFrame();
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
    ImGuizmo::SetRect(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

    glm::mat4 m{1.0f};
    auto *view = glm::value_ptr(camera.View());
    auto *proj = glm::value_ptr(camera.Projection());
    
    ImGuizmo::DrawGrid(view, proj, glm::value_ptr(m), 100.0f);

    cameracontroller.Debug();

    ImGui::Begin("Controlls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Checkbox("Paused", &time.paused);
    ImGui::SliderFloat("Time Factor", &time.factor, 0.0f, 10.0f);
    ImGui::ColorEdit3("Light color", &light_color.x);
    ImGui::ColorEdit3("Water color", &debug.water_color.x);



    /* build debug ui here */

    ImGui::End();
}