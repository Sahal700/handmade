#include <windows.h>
#include <stdint.h>
#include <dsound.h>

#define local_presist static
#define internal static
#define global_variable static

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice,LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);


struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;

struct win32_window_dimension
{
    int Width;
    int Height;
};

win32_window_dimension win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
    return Result;
}

internal void RenderGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
    uint8_t *Row = (uint8_t *)Buffer->Memory;
    for (int Y = 0; Y < Buffer->Height; ++Y)
    {

        uint32_t *Pixel = (uint32_t *)Row;
        for (int X = 0; X < Buffer->Width; ++X)
        {
            uint8_t BlUE = X + XOffset;
            uint8_t Green = Y + YOffset;

            *Pixel++ = (Green << 8 | BlUE);
        }
        Row += Buffer->Pitch;
    }
}

internal void win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->BytesPerPixel = 4;

    // NOTE: when the biHeight field id -ve, this is the clue to windows
    // to treat this bit-map as top-down, not bottom-up, meaning that
    // the first three bytes of the image are the color for the lop left in
    // the bitmap note the bottom left
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width * Buffer->BytesPerPixel;

    // TODO: probabaly clear this to black
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, win32_offscreen_buffer *Buffer)
{
    // TODO: correct aspect ratio
    StretchDIBits(DeviceContext,
                  //   X, Y, Width, Height,
                  //   X, Y, Width, Height,
                  0, 0, WindowWidth, WindowHeight,
                  0, 0, Buffer->Width, Buffer->Height,
                  Buffer->Memory,
                  &Buffer->Info,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}

internal void Win32InitDSound(void)
{
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    if (DSoundLibrary)
    {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *) GetProcAddress(DSoundLibrary,"DirectSoundCreate");
        LPDIRECTSOUND DirectSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        { 
            
        }else
        {
            // TODO: Error log
        }
        
    }
    

}

LRESULT CALLBACK Win32WndprocCallback(
    HWND Window,
    UINT Message,
    WPARAM Wparam,
    LPARAM Lparam)
{
    LRESULT Result = 0;

    switch (Message)
    {
    case WM_SIZE:
    {
    }
    break;
    case WM_DESTROY:
    {
        GlobalRunning = false;
    }
    break;
    case WM_CLOSE:
    {
        GlobalRunning = false;
    }
    break;
    case WM_ACTIVATEAPP:
    {
        OutputDebugStringA("WM_ACTIVATEAPP\n");
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(Window, &Paint);

        RECT ClientRect;
        win32_window_dimension Dimension = win32GetWindowDimension(Window);
        Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackBuffer);
        EndPaint(Window, &Paint);
    }
    break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        uint32_t VKCode = Wparam;
        bool WasDown = ((Lparam & (1 << 30)) != 0);
        bool IsDown = ((Lparam & (1 << 31)) == 0);
        if (WasDown != IsDown)
        {
            
            if (VKCode == 'W')
            {
                OutputDebugStringA("W:");
                if (WasDown)
                {
                    OutputDebugStringA("was dow ");
                }
                if (IsDown)
                {
                    OutputDebugStringA("is down ");
                }
                OutputDebugStringA("\n");
            }
            else if (VKCode == 'S')
            {
                OutputDebugStringA("S\n");
            }
            else if (VKCode == 'A')
            {
                OutputDebugStringA("A\n");
            }
            else if (VKCode == 'D')
            {
                OutputDebugStringA("D\n");
            }
            else if (VKCode == 'Q')
            {
                OutputDebugStringA("Q\n");
            }
            else if (VKCode == 'E')
            {
                OutputDebugStringA("E\n");
            }
            else if (VKCode == VK_UP)
            {
                OutputDebugStringA("Up arrow\n");
            }
            else if (VKCode == VK_DOWN)
            {
                OutputDebugStringA("Down arrow\n");
            }
            else if (VKCode == VK_LEFT)
            {
                OutputDebugStringA("Left arrow\n");
            }
            else if (VKCode == VK_RIGHT)
            {
                OutputDebugStringA("Right arrow\n");
            }
            else if (VKCode == VK_ESCAPE)
            {
                OutputDebugStringA("Escape\n");
            }
            else if (VKCode == VK_SPACE)
            {
                OutputDebugStringA("Space\n");
            }
        }
        bool AltKeyDown = ((Lparam & (1<<29)) != 0);
        if(VKCode == VK_F4 && AltKeyDown)
        {
            GlobalRunning = false;
        }
    }

    default:
    {
        // OutputDebugStringA("WM_SIZE\n");
        Result = DefWindowProcA(Window, Message, Wparam, Lparam);
    }
    break;
    }
    return (Result);
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    WNDCLASSA WindowClass = {};

    win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32WndprocCallback;
    // WindoClass.hIcon = ;
    WindowClass.hInstance = hInst;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            hInst,
            0);
        if (Window)
        {

            GlobalRunning = true;
            int XOffset = 0;
            int YOffset = 0;

            Win32InitDSound();

            while (GlobalRunning)
            {
                MSG Message;

                while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        GlobalRunning = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
                RenderGradient(&GlobalBackBuffer, XOffset, YOffset);

                HDC DeviceContext = GetDC(Window);
                win32_window_dimension Dimension = win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackBuffer);
                ReleaseDC(Window, DeviceContext);

                ++XOffset;
                ++YOffset;
            }
        }
        else
        {
            // to do
        }
    }
    else
    {
        // to do
    }
    return 0;
}