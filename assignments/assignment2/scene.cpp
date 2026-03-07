#include "scene.h"
#include <iostream>

// imgui
#include "imgui/imgui.h"
#include "imguizmo/imguizmo.h"

// glm
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

// batteries
#include "batteries/opengl.h"

using namespace std;

struct {
    float shininess = 1.0f;
    glm::vec3 diffuse = glm::vec3(0, 0, 0);//glm::vec3(0.5, 0, 0.5);
    glm::vec3 specular = glm::vec3(0, 0, 0);//glm::vec3(0, 0.5, 0.5);
    glm::vec3 ambience = glm::vec3(0, 0, 0);//glm::vec3(0, 0.5, 0.5);
} debug;

struct FullScreenQuad 
{
    GLuint vao; 
    GLuint vbo; // array of vertices that constucts an object to render

    void Initialize()
    {
        float verticies[] = {
            // pos (x, y), texcoord (v,v)

            // traingle 1
            -1.0f, 1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f,

            // triangle 2
            -1.0f, 1.0f, 0.0f, 1.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao); // everything i bind after will be bound to the vao state
        glBindBuffer(GL_ARRAY_BUFFER, vbo); // vbo is now bound to vao stat
      
        glBufferData(GL_ARRAY_BUFFER, sizeof(verticies), &verticies, GL_STATIC_DRAW);

        // pos (x, y)
        // texcoord (u, v)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(sizeof(float) * 2));

        // always last.
        glBindVertexArray(0);
    }
};

FullScreenQuad fullscreen_quad;
static int selectedItem = 8; // Index of the currently selected item

float kernel_strength = 1.0f;
float vingette_strength = 0.25f;
float fisheye_radius = 7.55f;
float fisheye_scale = 0.75f;

float buffer_line_time_split = 1.8f;
float buffer_line_y_split = 0.1f;
float buffer_line_strength = 1.0f;

float buffer_circle_time_split = 1.8f;
float buffer_circle_dist_split = 0.1f;
float buffer_circle_strength = 1.0f;

float duplication_screen_divisor = 10.0f;

float min_shadow_bias = 0.005f;
float max_shadow_bias = 0.05f;

Scene::Scene()
{
    suzanne = std::make_unique<ew::Model>("assets/models/suzanne.obj");
    toon = std::make_unique<ew::Shader>(
        "assets/shaders/default.vs",
        "assets/shaders/toon.fs"
    );

    shadow = std::make_unique<ew::Shader>(
        "assets/shaders/shadow.vs",
        "assets/shaders/shadow.fs"
    );

    txGradient = std::make_unique<ew::Texture>(
        "assets/textures/ZAtoon.png"
    );

    postprocess = std::make_unique<ew::Shader>(
        "assets/shaders/fullscreen.vs",
        "assets/shaders/blur.fs"
    );


    // Setup Light Camera
    lightCamera.orthographic = true;
    lightCamera.aspectRatio = 1.0f;      
    lightCamera.orthoHeight = 10.0f;    
    lightCamera.nearPlane = 0.01f;
    lightCamera.farPlane = 50.0f;
    lightCamera.target = glm::vec3(0.0f);

    // Define Plane Mesh
    planeMesh = ew::Mesh(ew::createPlane(10, 10, 5));

    light = {
        .brightness = 1.0f,
        .color = {1.0f, 0.0f, 1.0f},
        .position = {2.0f, 2.0f, 2.0f},
    };

    palette = {
        .color1 = {1.0f, 0.0f, 0.0f},
        .color2 = {0.0f, 0.0f, 1.0f}
    };

    fullscreen_quad.Initialize();

    //framebuffer setup
    glCreateFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    {
        glGenTextures(1, &fboTexture);
        glBindTexture(GL_TEXTURE_2D, fboTexture);

        //Create 800/600 render texture with 8 unsigned bytes
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0);

        //Create depth texture
        glGenTextures(1, &fboDepth);
        glBindTexture(GL_TEXTURE_2D, fboDepth);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8,
            800, 600, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fboDepth, 0);
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0);  

    // Shadowbuffer Setup
    glCreateFramebuffers(1, &shadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    {
        //Create depth texture
        glGenTextures(1, &shadowMap);
        glBindTexture(GL_TEXTURE_2D, shadowMap);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
            1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);

        // Declare No Color AFTER Attatched
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);  
}

Scene::~Scene()
{
    glDeleteFramebuffers(1, &fbo);
}

void Scene::Update(float dt)
{
    batteries::Scene::Update(dt);

    // Update Light Camera Position To Be Opposite Light Direction
    lightCamera.position = light.position;
    lightCamera.target = glm::vec3(0.0f);   
    /* body */
}

glm::vec3 light_color = glm::vec3(1.0f);
void Scene::Render(void)
{
    // shadow pipeline
    glViewport(0, 0, 1024, 1024);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    {
        // Calculate Light Space Matrix
        lightSpaceMatrix = lightCamera.projectionMatrix() * lightCamera.viewMatrix();

        // Clear Last Frame
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT);

        // Use Shadow Shader & Set Internals
        shadow->use();
        shadow->setMat4("lightSpaceMatrix", lightSpaceMatrix);

        // Enable Depth Test BEFORE Draw
        glEnable(GL_DEPTH_TEST);

        // Set Base Model For Suzanne
        shadow->setMat4("model", glm::mat4(1.0f));

        // Draw Suzanne
        suzanne->draw();

        // Define Plane Model With Offset
        glm::mat4 planeModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        shadow->setMat4("model", planeModel);

        // Draw the Plane
        planeMesh.draw();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 800, 600);


    // suzanne pipeline
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    {
        const auto view_proj = camera.Projection() * camera.View();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);

        auto index = 0;
        glActiveTexture(GL_TEXTURE0 + index);
        glBindTexture(GL_TEXTURE_2D, txGradient->getID());
        // glDisable(GL_DEPTH_TEST);

        toon->use();
        toon->setInt("txGradient", index);
        
        // scene matrices
        toon->setMat4("view_proj", view_proj);
        toon->setVec3("camera", camera.position);

        toon->setFloat("material.shininess", debug.shininess);
        toon->setVec3("material.ambient", debug.ambience);
        toon->setVec3("material.diffuse", debug.diffuse);
        toon->setVec3("material.specular", debug.specular);
        
        toon->setVec3("light.position", light.position);
        toon->setVec3("light.color", light_color);

        toon->setVec3("pal.color1", palette.color1);
        toon->setVec3("pal.color2", palette.color2);

        toon->setMat4("lightSpaceMatrix", lightSpaceMatrix);
        toon->setInt("shadowMap", 1);

        toon->setFloat("minBias", min_shadow_bias);
        toon->setFloat("maxBias", max_shadow_bias);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, shadowMap);

        // Set Base Model For Suzanne
        toon->setMat4("model", glm::mat4(1.0f));

        // draw suzanne
        suzanne->draw();

        // Define Plane Model With Offset
        glm::mat4 planeModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        toon->setMat4("model", planeModel);

        // Draw the plane
        planeMesh.draw();

    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    
    { // post processing pipline
        // render fullscreen quad
        postprocess->use();
        postprocess->setInt("screen", 0);
        postprocess->setFloat("effect", selectedItem);

        postprocess->setFloat("kernel_strength", kernel_strength);
        postprocess->setFloat("vingette_strength", vingette_strength);
        postprocess->setFloat("fisheye_radius", fisheye_radius);
        postprocess->setFloat("fisheye_scale", fisheye_scale);

        postprocess->setFloat("time", time.absolute);

        postprocess->setFloat("buffer_line_time_split", buffer_line_time_split);
        postprocess->setFloat("buffer_line_y_split", buffer_line_y_split);
        postprocess->setFloat("buffer_line_strength", buffer_line_strength);

        postprocess->setFloat("buffer_circle_time_split", buffer_circle_time_split);
        postprocess->setFloat("buffer_circle_dist_split", buffer_circle_dist_split);
        postprocess->setFloat("buffer_circle_strength", buffer_circle_strength);

        postprocess->setFloat("duplication_screen_divisor", duplication_screen_divisor);

        // fullscreen pipeline
        glDisable(GL_DEPTH_TEST);

        // default framebuffer
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //draw fullscreenquad
        glBindVertexArray(fullscreen_quad.vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fboTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

void Scene::Debug(void)
{
    ImGuizmo::BeginFrame();
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
    ImGuizmo::SetRect(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

    glm::mat4 m{1.0f};
    auto *view = glm::value_ptr(camera.View());
    auto *proj = glm::value_ptr(camera.Projection());
    
    //ImGuizmo::DrawGrid(view, proj, glm::value_ptr(m), 100.0f);

    auto matrix = glm::translate(glm::mat4(1.0f), light.position);

    ImGuizmo::Manipulate(
        view,
        proj,
        ImGuizmo::TRANSLATE,
        ImGuizmo::WORLD,
        glm::value_ptr(matrix)
    );

    if(ImGuizmo::IsUsing())
    {
        light.position = glm::vec3(matrix[3]);
    }

    cameracontroller.Debug();

    ImGui::Begin("Controlls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::SliderFloat("Alpha", &debug.shininess, 1.0f, 128.0f);
    ImGui::ColorEdit3("Light color", &light_color.x);
    ImGui::ColorEdit3("Ambience color", &debug.ambience.x);
    ImGui::ColorEdit3("Diffuse color", &debug.diffuse.x);
    ImGui::ColorEdit3("Specular color", &debug.specular.x);

    ImGui::SeparatorText("Palette");
    ImGui::ColorEdit3("Color1", &palette.color1[0]);
    ImGui::ColorEdit3("Color2", &palette.color2[0]);

    ImGui::SliderFloat("MinShadowBias", &min_shadow_bias, 0.001f, 0.1f);
    ImGui::SliderFloat("MaxShadowBias", &max_shadow_bias, 0.01f, 0.5f);

    static const char* items[] = {
        "Light Ridge Detection", // 0
        "Heavy Ridge Detection", // 1
        "Sharpen", // 2 
        "Normalized Box Blur", // 3
        "Gaussian Blur", // 4
        "Square Vignette", // 5
        "Fisheye", // 6
        "Buffer Lines", // 7
        "Buffer Circles", // 8
        "Duplication"
     };

    ImGui::Combo("Processing Effect", &selectedItem, items, IM_ARRAYSIZE(items));

    if(selectedItem < 5)
    {
        ImGui::SliderFloat("Kernel Stength", &kernel_strength, -10.0f, 10.0f);
    }
    
    if(selectedItem == 5)
    {
        ImGui::SliderFloat("Vignette Stength", &vingette_strength, 0.0f, 0.5f);
    }
    
    if(selectedItem == 6)
    {
        ImGui::SliderFloat("Fisheye Radius", &fisheye_radius, 0.0f, 1.0f);
        ImGui::SliderFloat("Fisheye Scale", &fisheye_scale, 0.0f, 10.0f);
    }

    if(selectedItem == 7)
    {
        ImGui::SliderFloat("Time Split", &buffer_line_time_split, 0.01f, 10.0f);
        ImGui::SliderFloat("Y Split", &buffer_line_y_split, 0.01f, 1.0f);
        ImGui::SliderFloat("Strength", &buffer_line_strength, -10.0f, 10.0f);
    }

    if(selectedItem == 8)
    {
        ImGui::SliderFloat("Time Split", &buffer_circle_time_split, 0.01f, 10.0f);
        ImGui::SliderFloat("Distance Split", &buffer_circle_dist_split, 0.01f, 1.0f);
        ImGui::SliderFloat("Strength", &buffer_circle_strength, -10.0f, 10.0f);
    }
    
    if(selectedItem == 9)
    {
        ImGui::SliderFloat("Screen Divisions", &duplication_screen_divisor, 2.0f, 30.0f);
    }

    // Display Base Texture (Suzzane) 
    ImGui::Image(
        (void*)(intptr_t)fboTexture,
        ImVec2(400, 300),
        ImVec2(0, 1), ImVec2(1, 0));

    // Display Depth Buffer
    ImGui::Image(
        (void*)(intptr_t)fboDepth,
        ImVec2(400, 300),
        ImVec2(0, 1), ImVec2(1, 0));
        
    // Display Shadow Map
    ImGui::Image(
        (void*)(intptr_t)shadowMap,
        ImVec2(400, 300),
        ImVec2(0, 1), ImVec2(1, 0));

    // Stop ImGui 
    ImGui::End();
}