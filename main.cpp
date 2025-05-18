#include "window.hpp"
#include "VulkanRenderer.hpp"

void updatePosition(Window* window, glm::vec3* eye, glm::vec3* centerVec, glm::vec3* up, glm::vec3* centerDir);
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    Window* wnd = new Window(1000, 1000, L"yolo", L"test");
    VulkanRenderer* renderer = new VulkanRenderer(hInstance, wnd->GetWindowHWND());
    glm::vec3 eye{ 0.0f, 0.0f, 0.0f }, centerDir{ 0.0f, 0.0f, 1.0f }, up{ 0.0f, 1.0f, 0.0f };
    glm::vec3 center = eye + centerDir;
    while (wnd->ProcessMessages() == 0)
    {
        renderer->updateCameraLH(eye, center, up);
        renderer->updateRotation();
        renderer->Render();
        updatePosition(wnd, &eye, &center, &up, &centerDir);
    }
    
}

void updatePosition(Window* window, glm::vec3* eye, glm::vec3* center, glm::vec3* up, glm::vec3* centerDir)
{
    static float angleX = 0.0f, angleY =0.0f;
    if (window->IsKeyPressed('W')) { *eye += 0.0007f * (*centerDir); }
    if (window->IsKeyPressed('S')) { *eye -= 0.0007f * (*centerDir); }
    if (window->IsKeyPressed(VK_SPACE)) { *eye += 0.0007f * (*up); }
    if (window->IsKeyPressed(VK_CONTROL)) { *eye -= 0.0007f * (*up); }
    if (window->IsKeyPressed('D')){ *eye += 0.0007f * glm::cross(*up, *centerDir);}
    if (window->IsKeyPressed('A')){ *eye -= 0.0007f * glm::cross(*up, *centerDir); }
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
            angleX += 0.005f  *  (abs(angleX) > 3.14f/2.0f ? -1.0f : 1.0f );
        }
        else if (window->GetMouseDeltaY() > 0)
        {
            angleX -= 0.005f * (abs(angleX) > 3.14f / 2.0f ? -1.0f : 1.0f);
        }
        // IMPORTANT: Order MATTERS !!!!
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angleX, glm::vec3(1.0f, 0.0f, 0.0f)) *
                        glm::rotate(glm::mat4(1.0f), angleY, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::vec4 pos(0.0f, 0.0f, 1.0f, 0.0f);
        pos = pos * rot;
        *centerDir = glm::vec3(pos);
    }
    *center = *eye + *centerDir;
}