#include "ImageFile.h"
#include <objidl.h>
#include <gdiplusenums.h>
#include <gdiplus.h>
#include <assert.h>
#include <thread>
#include <queue>
#include <mutex>
#pragma comment (lib,"Gdiplus.lib")

#define THREAD_COUNT 8
struct TaskDescriptor
{
    std::wstring path;
    ImageFile* loadble;
};

struct GlobalState
{
    std::thread loadingThread[THREAD_COUNT];
    volatile bool terminateThread[THREAD_COUNT];
    volatile bool doingWork[THREAD_COUNT];
    std::queue<TaskDescriptor> loadableQueue;
    std::mutex queueAccesMutes;
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
} *globalState;

static void loadImage(GlobalState* state, const int threadIdx);

ImageFile::ImageFile(unsigned int* FileBuff, int width, int height)
    :
    width(width), height(height)
{
    this->FileBuff = new unsigned int[width * height];
    memcpy(this->FileBuff, FileBuff, width * height * sizeof(unsigned int));
}

ImageFile::ImageFile(const std::wstring filename)
    :
    loadingIsFinished(false)
{
    TaskDescriptor task;
    task.path = std::move(filename);
    task.loadble = this;

    globalState->queueAccesMutes.lock();
    globalState->loadableQueue.push(task);
    globalState->queueAccesMutes.unlock();

}

unsigned int ImageFile::GetColorAt(int x, int y) const
{
    return FileBuff[y * width + x];
}

unsigned int* const ImageFile::GetFilePtr() const
{
    return FileBuff;
}

const int ImageFile::GetWidth() const noexcept
{
    return width;
}

const int ImageFile::GetHeight() const noexcept
{
    return height;
}


ImageFile::~ImageFile()
{
    if (FileBuff != nullptr)
    {
        delete[] FileBuff;
        FileBuff = nullptr;
    }
}

void ImageFile::InitModule()
{
    if (globalState == nullptr)
    {
        globalState = new GlobalState();
        GdiplusStartup(&globalState->gdiplusToken, &globalState->gdiplusStartupInput, nullptr);
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            globalState->loadingThread[i] = std::thread(loadImage, globalState, i);
            
        }
    }
}

void ImageFile::CloseModule()
{
    if (globalState != nullptr)
    {
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            globalState->terminateThread[i] = true;
        }

        for (int i = 0; i < THREAD_COUNT; i++)
        {
            globalState->loadingThread[i].join();
        }

        Gdiplus::GdiplusShutdown(globalState->gdiplusToken);
        delete globalState;
        globalState = nullptr;
    }
}

bool ImageFile::AllFinished()
{
    return false;
}

static void loadImage(GlobalState* state, const int threadIdx)
{
    while (state->terminateThread[threadIdx] == false)
    {
        TaskDescriptor task;
        while (true)
        {
            state->queueAccesMutes.lock();
            if (state->loadableQueue.size() > 0)
            {

                task = state->loadableQueue.front();
                state->loadableQueue.pop();
                state->queueAccesMutes.unlock();
                break;
            }
            state->queueAccesMutes.unlock();
            Sleep(200);
        }

        Gdiplus::Bitmap bmp(task.path.c_str());

        assert(bmp.GetLastStatus() == Gdiplus::Status::Ok);

        task.loadble->height = bmp.GetHeight();
        task.loadble->width = bmp.GetWidth();

        task.loadble->FileBuff = new unsigned int[task.loadble->width * task.loadble->height];

        for (int y = 0; y < task.loadble->height; y++)
            for (int x = 0; x < task.loadble->width; x++)
            {
                Gdiplus::Color col;
                bmp.GetPixel(x, y, &col);
                unsigned int color = (unsigned int)col.GetRed() |
                    col.GetGreen() << 8 | col.GetBlue() << 16 | col.GetAlpha() << 24;
                task.loadble->FileBuff[y * task.loadble->width + x] = color;
            }
        task.loadble->loadingIsFinished = true;
    }
}
