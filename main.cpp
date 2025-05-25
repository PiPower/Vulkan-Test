#include "window.hpp"
#include "VulkanRenderer.hpp"
#include "Scene.hpp"
#include <chrono>
using namespace std;
void updatePosition(float dt, Window* window, glm::vec3* eye, glm::vec3* centerVec, glm::vec3* up, glm::vec3* centerDir, glm::vec3* upLook);
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    Window* wnd = new Window(1600, 900, L"yolo", L"test");
    VulkanRenderer* renderer = new VulkanRenderer(hInstance, wnd->GetWindowHWND(), "D:\\main1_sponza\\NewSponza_Main_glTF_003.gltf");
    glm::vec3 eye{ 0.0f, 10.0f, -10.0f }, centerDir{ 0.0f, 0.0f, 1.0f }, up{ 0.0f, 1.0f, 0.0f }, upLook{ 0.0f, 1.0f, 0.0f };
    glm::vec3 center = eye + centerDir;
    renderer->updateRotation();

    while (wnd->ProcessMessages() == 0)
    {
        auto t1 = chrono::high_resolution_clock::now();
        renderer->updateCameraLH(eye, center, upLook);
        renderer->Render();
        auto t2 = chrono::high_resolution_clock::now();

        chrono::duration duration = t2 - t1;
        float dt = (float)duration.count() / 1'000'000'000.0f;
        updatePosition(dt, wnd, &eye, &center, &up, &centerDir, &upLook);
    }
    
}

void updatePosition(float dt, Window* window, glm::vec3* eye, glm::vec3* center, glm::vec3* up, glm::vec3* centerDir, glm::vec3* upLook)
{
    static float angleX = 0.0f, angleY =0.0f;
    if (window->IsKeyPressed('W')) { *eye += dt * 10.0f * (*centerDir); }
    if (window->IsKeyPressed('S')) { *eye -= dt * 10.0f * (*centerDir); }
    if (window->IsKeyPressed(VK_SPACE)) { *eye += dt * 10.0f * (*up); }
    if (window->IsKeyPressed(VK_CONTROL)) { *eye -= dt * 10.0f * (*up); }
    if (window->IsKeyPressed('D')){ *eye += dt * 10.0f * glm::cross(*upLook, *centerDir);}
    if (window->IsKeyPressed('A')){ *eye -= dt * 10.0f * glm::cross(*upLook, *centerDir); }
    if (window->IsLeftPressed())
    {
        if (window->GetMouseDeltaX() < 0)
        {
            angleY -= dt * 10.0f;
        }
        else if (window->GetMouseDeltaX() > 0)
        {
            angleY += dt * 10.0f;
        }

        if (window->GetMouseDeltaY() < 0)
        {
            angleX += dt * 10.0f;
        }
        else if (window->GetMouseDeltaY() > 0)
        {
            angleX -= dt * 10.0f;
        }
        // IMPORTANT: Order MATTERS !!!!
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angleX, glm::vec3(1.0f, 0.0f, 0.0f)) *
                        glm::rotate(glm::mat4(1.0f), angleY, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::vec4 pos(0.0f, 0.0f, 1.0f, 0.0f);
        glm::vec4 upCamera(0.0f, 1.0f, 0.0f, 0.0f);
        pos = pos * rot;
        upCamera = upCamera * rot;
        *centerDir = glm::vec3(pos);
        *upLook = glm::vec3(upCamera);
    }
    *center = *eye + *centerDir;
}