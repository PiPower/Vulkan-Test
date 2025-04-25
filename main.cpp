#include "window.hpp"
#include "VulkanRenderer.hpp"

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    Window* wnd = new Window(1000, 1000, L"yolo", L"test");

    VulkanRenderer* renderer = new VulkanRenderer(hInstance, wnd->GetWindowHWND());
    while (wnd->ProcessMessages() == 0)
    {
        renderer->Render();
        renderer->updateRotation();
    }

}