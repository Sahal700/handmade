#include <windows.h>
#include <stdint.h>
#include <dsound.h>
#include <xaudio2.h>
#include <math.h>
#include <stdio.h>

#define local_persist static
#define internal static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;


typedef float  real32;
typedef double real64;

#include "handmade.cpp"

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice,LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);


struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable IXAudio2SourceVoice *SourceVoice;
global_variable XAUDIO2_BUFFER AudioBuffer;
global_variable int16 * AudioData;

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
    uint8 *Row = (uint8 *)Buffer->Memory;
    for (int Y = 0; Y < Buffer->Height; ++Y)
    {

        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0; X < Buffer->Width; ++X)
        {
            uint8 BlUE = X + XOffset;
            uint8 Green = Y + YOffset;

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
    int BytesPerPixel = 4;

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

    int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width * BytesPerPixel;

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

// internal void Win32InitDSound(HWND Window)
// {
//     HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
//     if (DSoundLibrary)
//     {
//         direct_sound_create *DirectSoundCreate = (direct_sound_create *) GetProcAddress(DSoundLibrary,"DirectSoundCreate");
//         LPDIRECTSOUND DirectSound;
//         if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
//         { 
//             if(SUCCEEDED( DirectSound->SetCooperativeLevel(Window,DSSCL_PRIORITY)))
//             {

//             }else
//             {
//                 // TODO: Error log
//             }
//         }else
//         {
//             // TODO: Error log
//         }
        
//     }
// }

internal void win32InitXAudio(){
    IXAudio2 *XAudio;
    
    if (SUCCEEDED(XAudio2Create(&XAudio,0,XAUDIO2_DEFAULT_PROCESSOR)))
    {
        IXAudio2MasteringVoice * MasteringVoice;
        if(SUCCEEDED(XAudio->CreateMasteringVoice(&MasteringVoice,2,48000,0,0)))
        {

            tWAVEFORMATEX WaveFormat ={};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = 48000;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) /8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;

            
            SourceVoice;
           
            if (SUCCEEDED(XAudio->CreateSourceVoice(&SourceVoice, &WaveFormat)))
            {
                int SamplesPerSecond = 48000;
                int ToneHz           = 256;
                int SampleCount      = SamplesPerSecond;

                AudioData = (int16 *)VirtualAlloc(0, SampleCount * WaveFormat.nBlockAlign, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
                

                int16 *SampleOut = AudioData;
                int Period = SamplesPerSecond / ToneHz;
                int16 Volume = 3000;

                for (int SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
                {
                    float t = (float)SampleIndex / (float)SamplesPerSecond;
                    float Frequency = 440.0f; // A4 note
                    int16 SampleValue = (int16)(Volume * sinf(2.0f * 3.14159f * Frequency * t));
                    *SampleOut++ = SampleValue; 
                    *SampleOut++ = SampleValue;
                }

                AudioBuffer.AudioBytes =WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign,
                AudioBuffer.pAudioData = (BYTE *)AudioData;
                AudioBuffer.Flags      = XAUDIO2_END_OF_STREAM;
                AudioBuffer.LoopCount = XAUDIO2_LOOP_INFINITE;

                SourceVoice->SubmitSourceBuffer(&AudioBuffer);
                SourceVoice->Start(0);
            }

        }else
        {

        }
    }else
    {
        // TODO error handling
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
        uint32 VKCode = Wparam;
        bool WasDown = ((Lparam & (1 << 30)) != 0);
        bool IsDown = ((Lparam & (1 << 31)) == 0);
        if (WasDown != IsDown)
        {
            
            if (VKCode == 'W')
            {
                OutputDebugStringA("W:");
                if (WasDown)
                {
                    OutputDebugStringA("was down");
                }
                if (IsDown)
                {
                    OutputDebugStringA("is down");
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

    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = Win32WndprocCallback;
    // WindoClass.hIcon = ;
    WindowClass.hInstance = hInst;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    LARGE_INTEGER PreCounterFrequencyResult;
    
    QueryPerformanceFrequency(&PreCounterFrequencyResult);
    int64 PreCounterFrequency = PreCounterFrequencyResult.QuadPart;

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
            // NOTE: since we specified CS_OWNDC in style tags
            // we can get context once and use it every were
            // becuase we are not sharing context with anyone else
            HDC DeviceContext = GetDC(Window);

            int XOffset = 0;
            int YOffset = 0;

            win32InitXAudio();

            GlobalRunning = true;

            uint64 LastCycle = __rdtsc();
            LARGE_INTEGER LastCounter;
            QueryPerformanceCounter(&LastCounter); 
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
                game_offscreen_buffer Buffer = {};
                Buffer.Memory = GlobalBackBuffer.Memory;
                Buffer.Width = GlobalBackBuffer.Width;
                Buffer.Height = GlobalBackBuffer.Height;
                Buffer.Pitch  = GlobalBackBuffer.Pitch; 
                GameUpdateRende(&Buffer,XOffset,YOffset);

                
                win32_window_dimension Dimension = win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackBuffer);
                ReleaseDC(Window, DeviceContext);

                ++XOffset;
                ++YOffset;

                uint64 EndCycle = __rdtsc();
                uint64 CycleElapsed = EndCycle - LastCycle;

                LARGE_INTEGER EndCounter;
                QueryPerformanceCounter(&EndCounter); 
                int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;  
                real32 MSPerFrame = ((1000.0f*(real32)CounterElapsed)/(real32)PreCounterFrequency);
                real32 FPS = (real32)PreCounterFrequency/(real32)CounterElapsed;
                real32 MCPF = ((real32)CycleElapsed/(1000.0f*1000.0f));

                char StrBuffer[256];
                sprintf(StrBuffer,"%.02fms %.02fFPS %.02fc\n",MSPerFrame,FPS,MCPF);
                OutputDebugStringA(StrBuffer);

                LastCounter = EndCounter;
                LastCycle = EndCycle;

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