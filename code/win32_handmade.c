/*
  TODO(casey): THIS IS NOT THE FINAL PLATFORM LAYER
  [ ] Saved game locations
  [ ] Getting a handle to our own executable file
  [ ] Asset loading path
  [ ] Threading (launch a thread)
  [ ] Raw input (support for multiple keyboards)
  [ ] Sleep/timeBeginPeriod
  [ ] ClipCursor() (for miltimonitor support)
  [ ] Fullscreen support
  [ ] WM_SETCURSOR (control cursor visibility)
  [ ] QueryCancelAutoplay
  [ ] WM_ACTIVATEAPP (for when we are not the active application)
  [ ] Blit speed improvements (BitBlt)
  [ ] Hardware acceleration (OpenGL or Direct3D or both?)
  [ ] GetKeyboardLayout (for French keyboards, internaltional WASD support)

  And more!
*/

#pragma warning(push, 3)
#include <dsound.h>
#include <windows.h>
#pragma warning(pop)
#include <stdbool.h>
#include <xinput.h>

#include "handmade.c"
#include "handmade.h"
#include "win32_handmade.h"

#pragma warning(disable : 5045)  // Qspectre

// NOTE: `running` is global for now
global_variable bool running;

global_variable Win32OffscreenBuffer gameBackBuffer = {.bytesPerPixel = 4};
global_variable int stuffx;
global_variable int stuffy;

/* XInpput: Start */

#define X_INPUT_GET_STATE(func) DWORD WINAPI func(DWORD dwUserIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(func) DWORD WINAPI func(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)

// TODO(daniel): Try this with function pointer instead
typedef X_INPUT_GET_STATE(sigXInputGetState);
typedef X_INPUT_SET_STATE(sigXInputSetState);

#pragma warning(disable : 4100)  // unused parameters
X_INPUT_GET_STATE(stubXInputGetState) { return ERROR_DEVICE_NOT_CONNECTED; }
X_INPUT_SET_STATE(stubXInputSetState) { return ERROR_DEVICE_NOT_CONNECTED; }
#pragma warning(default : 4100)

global_variable sigXInputGetState *dynamicXInputGetState = stubXInputGetState;
#define XInputGetState dynamicXInputGetState

global_variable sigXInputSetState *dynamicXInputSetState = stubXInputSetState;
#define XInputSetState dynamicXInputSetState

internal void Win32LoadXInput(void) {
  // TODO(casey): Test this on Windows 8
  HMODULE XInputModule = LoadLibraryA("xinput1_4.dll");
  if (XInputModule == NULL) {
    // TODO(casey): Diagnostic
    XInputModule = LoadLibraryA("xinput1_3.dll");
  }
  if (XInputModule == NULL) {
    XInputModule = LoadLibraryA("xinput9_1_0.dll");
  }

  if (XInputModule != NULL) {
    XInputGetState = (sigXInputGetState *)GetProcAddress(XInputModule, "XInputGetState");
    if (XInputGetState == NULL) {
      XInputGetState = stubXInputGetState;
    }

    XInputSetState = (sigXInputSetState *)GetProcAddress(XInputModule, "XInputSetState");
    if (XInputSetState == NULL) {
      XInputSetState = stubXInputSetState;
    }
  } else {
    // TODO(casey): Diagnostic
  }
}

internal void Win32ProcessXInputDigitalButton(WORD buttons,
                                              WORD buttonBit,
                                              GameButtonState *previousState,
                                              GameButtonState *currentState) {
  currentState->endedDown = ((buttons & buttonBit) > 0);
  currentState->halfTransitionCount = (previousState->endedDown != currentState->endedDown) ? 1 : 0;
}

/* XInpput: End */

/* DirectSound: Start */

typedef HRESULT WINAPI sigDirectSoundCreate(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);

internal void Win32InitDirectSound(HWND windowHandle, Win32SoundOutput *sound) {
  // NOTE(daniel): Sound buffer looks like this
  //  i16  i16    i16  i16    i16  i16  ...
  // LEFT RIGHT [LEFT RIGHT] LEFT RIGHT ...
  //    sample: |<==>|
  //    frame:  |<========>|

  // Calculated computed fields
  sound->bytesPerFrame = sound->nChannels * sound->bytesPerSample;
  sound->bufferSize = sound->bytesPerFrame * sound->samplesPerSec;

  // Load the library
  HMODULE DSoundModule = LoadLibraryA("dsound.dll");
  if (DSoundModule == NULL) {
    // TODO(casey): Diagnostic
    return;
  }

  sigDirectSoundCreate *DirectSoundCreate = (sigDirectSoundCreate *)GetProcAddress(DSoundModule, "DirectSoundCreate");
  if (DirectSoundCreate == NULL) {
    // TODO(casey): Diagnostic
    return;
  }

  // TODO(casey): Double-check this works on XP - DSound 7 or 8?
  LPDIRECTSOUND directSound;
  if (FAILED(DirectSoundCreate(NULL, &directSound, NULL))) {
    // TODO(casey): Diagnostic
    return;
  }

  if (FAILED(IDirectSound_SetCooperativeLevel(directSound, windowHandle, DSSCL_PRIORITY))) {
    // TODO(casey): Diagnostic
    return;
  }

  WAVEFORMATEX waveFormat = {0};
  waveFormat.wFormatTag = WAVE_FORMAT_PCM;
  waveFormat.nChannels = sound->nChannels;
  waveFormat.wBitsPerSample = (sound->bytesPerSample * 8);
  waveFormat.nBlockAlign = sound->bytesPerFrame;
  waveFormat.nSamplesPerSec = sound->samplesPerSec;
  waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

  // NOTE(daniel): Not a buffer! Just handle to the primary sound device
  DSBUFFERDESC primaryBufferDescription = {0};
  primaryBufferDescription.dwSize = sizeof(DSBUFFERDESC);
  primaryBufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
  LPDIRECTSOUNDBUFFER primaryBuffer;
  if (SUCCEEDED(IDirectSound_CreateSoundBuffer(directSound, &primaryBufferDescription, &primaryBuffer, NULL))) {
    if (FAILED(IDirectSoundBuffer_SetFormat(primaryBuffer, &waveFormat))) {
      // TODO(casey): Diagnostic
    }
  } else {
    // TODO(casey): Diagnostic
  }

  DSBUFFERDESC soundBufferDescription = (DSBUFFERDESC){
      .dwSize = sizeof(DSBUFFERDESC),
      .dwFlags = 0,
      .dwBufferBytes = sound->bufferSize,
      .dwReserved = 0,
      .lpwfxFormat = &waveFormat,
  };
  if (FAILED(IDirectSound_CreateSoundBuffer(directSound, &soundBufferDescription, &sound->buffer, NULL))) {
    // TODO(casey): Diagnostic
    return;
  }
}

internal void Win32ClearSoundBuffer(Win32SoundOutput *sound) {
  LPVOID lockRegion1;
  DWORD lockRegion1Bytes;
  LPVOID lockRegion2;
  DWORD lockRegion2Bytes;
  HRESULT lockError = IDirectSoundBuffer_Lock(
      sound->buffer, 0, sound->bufferSize, &lockRegion1, &lockRegion1Bytes, &lockRegion2, &lockRegion2Bytes, 0);
  // TODO(casey): Assert region 1 and 2 byte count are valid
  if (lockError == DS_OK) {
    u8 *bytes = (u8 *)lockRegion1;
    int region1FrameCount = lockRegion1Bytes / sound->bytesPerFrame;
    for (int i = 0; i < region1FrameCount; i += 1) {
      *bytes++ = 0;
    }

    bytes = (u8 *)lockRegion2;
    int region2FrameCount = lockRegion2Bytes / sound->bytesPerFrame;
    for (int i = 0; i < region2FrameCount; i += 1) {
      *bytes++ = 0;
    }

    if (FAILED(
            IDirectSoundBuffer_Unlock(sound->buffer, lockRegion1, lockRegion1Bytes, lockRegion2, lockRegion2Bytes))) {
      OutputDebugStringA("Error:Unlock");
    }
  }
}

internal void Win32FillSoundBuffer(Win32SoundOutput *sound,
                                   DWORD lockOffset,
                                   DWORD lockBytes,
                                   GameSoundBuffer *gameSound) {
  LPVOID lockRegion1;
  DWORD lockRegion1Bytes;
  LPVOID lockRegion2;
  DWORD lockRegion2Bytes;
  HRESULT lockError = IDirectSoundBuffer_Lock(
      sound->buffer, lockOffset, lockBytes, &lockRegion1, &lockRegion1Bytes, &lockRegion2, &lockRegion2Bytes, 0);
  // TODO(casey): Assert region 1 and 2 byte count are valid
  if (lockError == DS_OK) {
    // NOTE(daniel): We use i16 because .bitsPerSample is 16
    // If we want to try i8, then use 8 bits per sample
    i16 *srcSamples = gameSound->samples;

    i16 *destSamples = (i16 *)lockRegion1;
    DWORD region1FrameCount = lockRegion1Bytes / sound->bytesPerFrame;
    for (DWORD frameIndex = 0; frameIndex < region1FrameCount; ++frameIndex) {
      *destSamples++ = *srcSamples++;
      *destSamples++ = *srcSamples++;
      sound->runningFrameIndex += 1;
    }

    destSamples = (i16 *)lockRegion2;
    DWORD region2FrameCount = lockRegion2Bytes / sound->bytesPerFrame;
    for (DWORD frameIndex = 0; frameIndex < region2FrameCount; ++frameIndex) {
      *destSamples++ = *srcSamples++;
      *destSamples++ = *srcSamples++;
      sound->runningFrameIndex += 1;
    }

    if (FAILED(
            IDirectSoundBuffer_Unlock(sound->buffer, lockRegion1, lockRegion1Bytes, lockRegion2, lockRegion2Bytes))) {
      OutputDebugStringA("Error:Unlock");
    }
  } else {
  }
}

/* DirectSound: End*/

internal Win32WindowDimensions Win32GetWindowDimension(HWND windowHandle) {
  RECT clientRect;
  GetClientRect(windowHandle, &clientRect);

  return (Win32WindowDimensions){
      .width = clientRect.right - clientRect.left,
      .height = clientRect.bottom - clientRect.top,
  };
}

internal void Win32ResizeDIBSection(Win32OffscreenBuffer *buffer, int width, int height) {
  if (buffer->memory != NULL) {
    VirtualFree(buffer->memory, 0, MEM_RELEASE);
  }

  buffer->width = width;
  buffer->height = height;
  buffer->pitch = width * buffer->bytesPerPixel;

  buffer->info = (BITMAPINFO){
      .bmiHeader =
          {
              .biSize = sizeof(BITMAPINFOHEADER),
              .biWidth = width,
              .biHeight = -height,  // top-down image
              .biPlanes = 1,
              .biBitCount = (WORD)buffer->bytesPerPixel * 8,
              .biCompression = BI_RGB,
          },
  };

  SIZE_T bitmapMemorySize = ((SIZE_T)width * (SIZE_T)height * buffer->bytesPerPixel);
  buffer->memory = VirtualAlloc(NULL, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (buffer->memory == NULL) {
    OutputDebugStringA("Error:VirtualAlloc");
  }
}

internal void Win32CopyBufferToWindow(HDC deviceContext,
                                      LONG windowWidth,
                                      LONG windowHeight,
                                      Win32OffscreenBuffer *buffer) {
  StretchDIBits(deviceContext,  // handle to device context
                0,
                0,
                windowWidth,
                windowHeight,  // destination rectangle
                0,
                0,
                buffer->width,
                buffer->height,  // source rectangle
                buffer->memory,
                &buffer->info,  // pointer to DIB and its info
                DIB_RGB_COLORS,
                SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(_In_ HWND windowHandle,
                                         _In_ UINT messageID,
                                         _In_ WPARAM wParam,
                                         _In_ LPARAM lParam) {
  LRESULT result = 0;

  switch (messageID) {
    case WM_ACTIVATEAPP: {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    } break;

    case WM_CLOSE: {
      OutputDebugStringA("WM_CLOSE\n");
      if (MessageBoxA(windowHandle, "Close", "Handmade Hero", MB_YESNOCANCEL) == IDYES) {
        running = FALSE;
        DestroyWindow(windowHandle);
      }
    } break;

    case WM_CREATE: {
      OutputDebugStringA("WM_CREATE\n");
    } break;

    case WM_DESTROY: {
      OutputDebugStringA("WM_DESTROY\n");
      // NOTE: Handle this as an error (?)
      running = FALSE;
      PostQuitMessage(0);
    } break;

    case WM_PAINT: {
      PAINTSTRUCT paint;
      HDC deviceContext = BeginPaint(windowHandle, &paint);
      Win32WindowDimensions windowDims = Win32GetWindowDimension(windowHandle);

      Win32CopyBufferToWindow(deviceContext, windowDims.width, windowDims.height, &gameBackBuffer);
      TextOutA(deviceContext, 0, 0, "Hello, Windows!", 15);

      EndPaint(windowHandle, &paint);
    } break;

    case WM_SIZE: {
      OutputDebugString("WM_SIZE\n");
    } break;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP: {
      u8 vkCode = (u8)wParam;
      bool wasDown = ((lParam >> 30) & 0x1) != 0;
      bool isDown = ((lParam >> 31) & 0x1) == 0;

      switch (vkCode) {
        case 'C': {
          OutputDebugStringA("C: ");
          if (isDown) OutputDebugStringA("isDown ");
          if (wasDown) OutputDebugStringA("wasDown");
          OutputDebugStringA("\n");
        } break;
        case 'W': {
          stuffx += 16;
        } break;
        case 'S': {
          stuffx -= 16;
        } break;
        case 'A': {
          stuffy += 16;
        } break;
        case 'D': {
          stuffy -= 16;
        } break;
        default:
          break;
      }

      if (GetAsyncKeyState(VK_ESCAPE)) {
        OutputDebugStringA("WM_KEYDOWN:VK_ESCAPE\n");
        SendMessageA(windowHandle, WM_CLOSE, 0, 0);
      }

      BOOL altKeyDown = (lParam >> 29) & 0x1;
      if (altKeyDown && (vkCode == VK_F4)) {
        running = FALSE;
      }
    } break;

    default: {
      result = DefWindowProcA(windowHandle, messageID, wParam, lParam);
    } break;
  }

  return result;
}

int WINAPI WinMain(_In_ HINSTANCE instance,
                   _In_opt_ HINSTANCE previousInstance,
                   _In_ LPSTR commandLine,
                   _In_ int commandShow) {
  LARGE_INTEGER performanceFrequency;
  QueryPerformanceFrequency(&performanceFrequency);
  LONGLONG countsPerSecond = performanceFrequency.QuadPart;

  Win32LoadXInput();

  Win32ResizeDIBSection(&gameBackBuffer, 1280, 720);

  WNDCLASSA windowClass = {
      .style = CS_HREDRAW | CS_VREDRAW,        // redraw window when size changes
      .lpfnWndProc = Win32MainWindowCallback,  // pointer to window procedure
      .hInstance = instance,
      .hIcon = NULL,  // handle to the class icon
      .lpszClassName = "HandmadeHero_WindowClass",
  };

  if (RegisterClassA(&windowClass) == 0) {
    // https://docs.microsoft.com/en-us/windows/win32/debug/retrieving-the-last-error-code
    OutputDebugStringA("Error:RegisterClassA\n");
    return 0;
  }

  HWND windowHandle = CreateWindowExA(0,                                 // extended window style
                                      windowClass.lpszClassName,         // window class name
                                      "Handmade Hero",                   // window name
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,  // window style
                                      CW_USEDEFAULT,                     // x
                                      CW_USEDEFAULT,                     // y
                                      CW_USEDEFAULT,                     // width
                                      CW_USEDEFAULT,                     // height
                                      0,                                 // parent window handle
                                      0,                                 // menu handle
                                      instance,                          // module's instance handle
                                      0                                  // pointer to value of WM_CREATE
  );
  if (windowHandle == NULL) {
    OutputDebugStringA("Error:CreateWindowExA\n");
    return 0;
  }

  Win32SoundOutput sound = {0};
  sound.nChannels = 2;
  sound.bytesPerSample = 2;  // 1 or 2
  sound.toneVolume = 3000;
  sound.samplesPerSec = 48000;
  sound.framesLatency = sound.samplesPerSec / 15;  // in seconds
  Win32InitDirectSound(windowHandle, &sound);
  Win32ClearSoundBuffer(&sound);
  IDirectSoundBuffer_Play(sound.buffer, 0, 0, DSBPLAY_LOOPING);

  MSG msg = {0};
  running = TRUE;

  // TODO(casey): Pool with bitmap VirtualAlloc
  i16 *samples = (i16 *)VirtualAlloc(NULL, sound.bufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (samples == NULL) {
    OutputDebugStringA("Error:VirtualAlloc-samples\n");
    return 0;
  }

  u64 previousCycleCount = __rdtsc();
  LARGE_INTEGER previousPerformanceCounter;
  QueryPerformanceCounter(&previousPerformanceCounter);

  GameInput previousGameInput = {0};
  int maxControllerCount = XUSER_MAX_COUNT;
  if (maxControllerCount > ARRAY_LENGTH(previousGameInput.controllers)) {
    maxControllerCount = ARRAY_LENGTH(previousGameInput.controllers);
  }

  GameMemory gameMemory = {0};
  gameMemory.permanent.size = MEGABYTES(64);
  gameMemory.transient.size = GIGABYTES(4);
  u64 gameMemoryTotalSize = gameMemory.permanent.size + gameMemory.transient.size;
#if HANDMADE_INTERNAL
  LPVOID gameMemoryBaseAddress = (LPVOID)TERABYTES(2);
#else
  LPVOID gameMemoryBaseAddress = NULL;
#endif
  LPVOID gameMemoryStorage =
      VirtualAlloc(gameMemoryBaseAddress, gameMemoryTotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (gameMemoryStorage == NULL) {
    OutputDebugStringA("Error:VirtualAlloc-GameMemory\n");
    return 0;
  }
  gameMemory.permanent.storage = gameMemoryStorage;
  gameMemory.transient.storage = (u8 *)gameMemoryStorage + gameMemoryTotalSize;

  while (running) {
    while (PeekMessageA(&msg, windowHandle, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        running = FALSE;
      }
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }

    GameInput gameInput = {0};
    // TODO(casey): Should we poll this more frequently?
    for (int controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; ++controllerIndex) {
      XINPUT_STATE controllerState = {0};

      GameControllerInput *previousController = &previousGameInput.controllers[controllerIndex];
      GameControllerInput *currentController = &gameInput.controllers[controllerIndex];

      if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS) {
        // Controller is connected
        // TODO(casey): see if .dwPacketNumber increments too rapidly
        XINPUT_GAMEPAD *gamepad = &controllerState.Gamepad;

        // TODO(casey): Deadzone handling properly
        // TODO(casey): Dpad

        f32 stickX = (f32)gamepad->sThumbLX;
        stickX /= (stickX < 0) ? 32768.0f : 32767.0f;
        f32 stickY = (f32)gamepad->sThumbLY;
        stickY /= (stickY < 0) ? 32768.0f : 32767.0f;

        currentController->isAnalog = TRUE;
        currentController->x.start = previousController->x.end;
        currentController->y.start = previousController->y.end;
        // TODO(casey): Min/Max macros!
        currentController->x.min = previousController->x.max = currentController->x.end = stickX;
        currentController->y.min = previousController->y.max = currentController->y.end = stickY;

        WORD buttons = gamepad->wButtons;
        Win32ProcessXInputDigitalButton(
            buttons, XINPUT_GAMEPAD_A, &previousController->button.down, &currentController->button.down);
        Win32ProcessXInputDigitalButton(
            buttons, XINPUT_GAMEPAD_B, &previousController->button.right, &currentController->button.right);
        Win32ProcessXInputDigitalButton(
            buttons, XINPUT_GAMEPAD_X, &previousController->button.left, &currentController->button.left);
        Win32ProcessXInputDigitalButton(
            buttons, XINPUT_GAMEPAD_Y, &previousController->button.up, &currentController->button.up);
        Win32ProcessXInputDigitalButton(buttons,
                                        XINPUT_GAMEPAD_LEFT_SHOULDER,
                                        &previousController->button.leftShoulder,
                                        &currentController->button.leftShoulder);
        Win32ProcessXInputDigitalButton(buttons,
                                        XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                        &previousController->button.rightShoulder,
                                        &currentController->button.rightShoulder);
      } else {
        // TODO(daniel): Controller is not connected
      }
    }

    // NOTE(casey): DirectSound output test
    // TODO(daniel): Might want to not do a thing if playCursor hasn't changed
    // between loops
    DWORD playCursor;
    DWORD writeCursor;
    DWORD targetCursor;
    DWORD lockOffset;
    DWORD lockBytes;
    b32 validSound = FALSE;
    if (SUCCEEDED(IDirectSoundBuffer_GetCurrentPosition(sound.buffer, &playCursor, &writeCursor))) {
      lockOffset = (sound.runningFrameIndex * sound.bytesPerFrame) % sound.bufferSize;
      targetCursor = (playCursor + (sound.framesLatency * sound.bytesPerFrame)) % sound.bufferSize;
      if (lockOffset <= targetCursor) {
        lockBytes = targetCursor - lockOffset;
      } else {
        lockBytes = (sound.bufferSize - lockOffset);
        lockBytes += targetCursor;
      }
      validSound = TRUE;
    }

    GameOffscreenBuffer gameScreen = {
        .memory = gameBackBuffer.memory,
        .width = gameBackBuffer.width,
        .height = gameBackBuffer.height,
        .pitch = gameBackBuffer.pitch,
    };

    // i16 *samples = (i16 *)_malloca(48000 * 2 * sizeof(i16));
    GameSoundBuffer gameSound = {
        .samplesPerSec = sound.samplesPerSec,
        .outputFramesCount = lockBytes / sound.bytesPerFrame,
        .samples = samples,
    };

    GameUpdateAndRender(&gameMemory, &gameScreen, &gameSound, &gameInput);

    if (validSound) {
      Win32FillSoundBuffer(&sound, lockOffset, lockBytes, &gameSound);
    }

    // CS_OWNDC:
    // https://devblogs.microsoft.com/oldnewthing/20060601-06/?p=31003
    HDC deviceContext = GetDC(windowHandle);
    Win32WindowDimensions windowDims = Win32GetWindowDimension(windowHandle);
    Win32CopyBufferToWindow(deviceContext, windowDims.width, windowDims.height, &gameBackBuffer);
    ReleaseDC(windowHandle, deviceContext);

    previousGameInput = gameInput;

    u64 cycleCount = __rdtsc();

    LARGE_INTEGER performanceCounter;
    QueryPerformanceCounter(&performanceCounter);

    u64 cyclesElapsed = cycleCount - previousCycleCount;
    i32 megaCyclesElapsed = (i32)(cyclesElapsed / 1000000);

    LONGLONG countsPerFrame = performanceCounter.QuadPart - previousPerformanceCounter.QuadPart;
    i32 fps = (i32)(countsPerSecond / countsPerFrame);
    i32 msPerFrame = (i32)(1000 * countsPerFrame / countsPerSecond);

#if 0
    char buffer[256];
    wsprintfA(buffer, "%d FPS, %dms/f, %dMc/f\n", fps, msPerFrame, megaCyclesElapsed);
    OutputDebugStringA(buffer);
#endif

    previousCycleCount = cycleCount;
    previousPerformanceCounter = performanceCounter;
  }

  return (int)msg.wParam;

  UNREFERENCED_PARAMETER(previousInstance);
  UNREFERENCED_PARAMETER(commandLine);
  UNREFERENCED_PARAMETER(commandShow);
}
