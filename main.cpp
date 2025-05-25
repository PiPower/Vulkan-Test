#include "window.hpp"
#include "VulkanRenderer.hpp"
#include "Scene.hpp"

void updatePosition(Window* window, glm::vec3* eye, glm::vec3* centerVec, glm::vec3* up, glm::vec3* centerDir, glm::vec3* upLook);
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    Window* wnd = new Window(1600, 900, L"yolo", L"test");
    VulkanRenderer* renderer = new VulkanRenderer(hInstance, wnd->GetWindowHWND());
    glm::vec3 eye{ 0.0f, 0.0f, -20.0f }, centerDir{ 0.0f, 0.0f, 1.0f }, up{ 0.0f, 1.0f, 0.0f }, upLook{ 0.0f, 1.0f, 0.0f };
    glm::vec3 center = eye + centerDir;
    renderer->updateRotation();

    renderer->loadScene("D:\\main1_sponza\\NewSponza_Main_glTF_003.gltf");

    while (wnd->ProcessMessages() == 0)
    {
        renderer->updateCameraLH(eye, center, upLook);
        renderer->Render();
        updatePosition(wnd, &eye, &center, &up, &centerDir, &upLook);
    }
    
}

void updatePosition(Window* window, glm::vec3* eye, glm::vec3* center, glm::vec3* up, glm::vec3* centerDir, glm::vec3* upLook)
{
    static float angleX = 0.0f, angleY =0.0f;
    if (window->IsKeyPressed('W')) { *eye += 0.005f * (*centerDir); }
    if (window->IsKeyPressed('S')) { *eye -= 0.005f * (*centerDir); }
    if (window->IsKeyPressed(VK_SPACE)) { *eye += 0.005f * (*up); }
    if (window->IsKeyPressed(VK_CONTROL)) { *eye -= 0.005f * (*up); }
    if (window->IsKeyPressed('D')){ *eye += 0.005f * glm::cross(*upLook, *centerDir);}
    if (window->IsKeyPressed('A')){ *eye -= 0.005f * glm::cross(*upLook, *centerDir); }
    if (window->IsLeftPressed())
    {
        if (window->GetMouseDeltaX() < 0)
        {
            angleY -= 0.005f;
        }
        else if (window->GetMouseDeltaX() > 0)
        {
            angleY += 0.005f;
        }

        if (window->GetMouseDeltaY() < 0)
        {
            angleX += 0.005f;
        }
        else if (window->GetMouseDeltaY() > 0)
        {
            angleX -= 0.005f;
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