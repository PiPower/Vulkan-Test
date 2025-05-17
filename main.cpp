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

    if (window->IsKeyPressed('W')) { *eye += 0.0007f * (*centerDir); }
    if (window->IsKeyPressed('S')) { *eye -= 0.0007f * (*centerDir); }
    if (window->IsKeyPressed('D'))
    {
        *eye += 0.0007f * glm::cross(*up, *centerDir);
        *center = *eye + *centerDir;
    }
    if (window->IsKeyPressed('A')) 
    { 
        *eye -= 0.0007f * glm::cross(*up, *centerDir);
        *center = *eye + *centerDir;
    }
    if (window->IsLeftPressed())
    {
        glm::mat4 rot = glm::mat4(1.0f);
        
        if (window->GetMouseDeltaX() < 0)
        {
            rot = glm::rotate(glm::mat4(1.0f), -0.005f, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else if (window->GetMouseDeltaX() > 0)
        {
            rot = glm::rotate(glm::mat4(1.0f), 0.005f, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        glm::vec4 pos(*centerDir, 0.0f);
        pos = pos * rot;
        *centerDir = glm::vec3(pos);
        *center = *eye + *centerDir;
    }

}