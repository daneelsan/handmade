#pragma warning(push, 3)
#include <windows.h>
#pragma warning(pop)
#include <stdint.h>

#define internal static
#define global_variable static
#define local_persist static

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// @NOTE: `running` is global for now
global_variable BOOL running;

typedef struct Win32OffscreenBuffer {
  void *memory;
  BITMAPINFO info;
  int width;
  int height;
  int bytesPerPixel;
  int pitch;

  u8 _padding[4];
} Win32OffscreenBuffer;

global_variable Win32OffscreenBuffer gameBackBuffer;

typedef struct Win32WindowDimensions {
  int width;
  int height;
} Win32WindowDimensions;

internal Win32WindowDimensions Win32GetWindowDimension(HWND windowHandle) {
  RECT clientRect;
  GetClientRect(windowHandle, &clientRect);

  return (Win32WindowDimensions){
      .width = clientRect.right - clientRect.left,
      .height = clientRect.bottom - clientRect.top,
  };
}

internal void renderStuff(Win32OffscreenBuffer buffer) {
  // @TODO: See what optimizer does in value vs reference
  local_persist int offset = 0;
  u8 *row = (u8 *)buffer.memory;

  for (int y = 0; y < buffer.height; ++y) {
    u32 *pixel = (u32 *)row;
    for (int x = 0; x < buffer.width; ++x) {
      *pixel = (u32)(x + y + offset);
      pixel += 1;
    }
    row += buffer.pitch;
  }

  ++offset;
}
internal void Win32ResizeDIBSection(Win32OffscreenBuffer *buffer, int width,
                                    int height) {
  if (buffer->memory != NULL) {
    VirtualFree(buffer->memory, 0, MEM_RELEASE);
  }

  buffer->width = width;
  buffer->height = height;
  buffer->bytesPerPixel = 4;
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

  SIZE_T bitmapMemorySize =
      ((SIZE_T)width * (SIZE_T)height * buffer->bytesPerPixel);
  buffer->memory =
      VirtualAlloc(NULL, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
  if (buffer->memory == NULL) {
    OutputDebugStringA("Error:VirtualAlloc");
  }
}

internal void Win32CopyBufferToWindow(HDC deviceContext, LONG windowWidth, LONG windowHeight,
                                      Win32OffscreenBuffer buffer) {
  StretchDIBits(deviceContext,                      // handle to device context
                0, 0, windowWidth, windowHeight,    // destination rectangle
                0, 0, buffer.width, buffer.height,  // source rectangle
                buffer.memory, &buffer.info,  // pointer to DIB and its info
                DIB_RGB_COLORS, SRCCOPY);
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
      if (MessageBoxA(windowHandle, "Close", "Handmade Hero", MB_YESNOCANCEL) ==
          IDYES) {
        running = FALSE;
        DestroyWindow(windowHandle);
      }
    } break;

    case WM_CREATE: {
      OutputDebugStringA("WM_CREATE\n");
    } break;

    case WM_DESTROY: {
      OutputDebugStringA("WM_DESTROY\n");
      // @NOTE: Handle this as an error (?)
      running = FALSE;
      PostQuitMessage(0);
    } break;

    case WM_KEYDOWN: {
      if (GetAsyncKeyState(VK_ESCAPE)) {
        OutputDebugStringA("WM_KEYDOWN:VK_ESCAPE\n");
        SendMessageA(windowHandle, WM_CLOSE, 0, 0);
      }
    } break;

    case WM_PAINT: {
      PAINTSTRUCT paint;
      HDC deviceContext = BeginPaint(windowHandle, &paint);
      Win32WindowDimensions windowDims = Win32GetWindowDimension(windowHandle);

      Win32CopyBufferToWindow(deviceContext, windowDims.width, windowDims.height, gameBackBuffer);
      TextOutA(deviceContext, 0, 0, "Hello, Windows!", 15);

      EndPaint(windowHandle, &paint);
    } break;

    case WM_SIZE: {
      OutputDebugString("WM_SIZE\n");
    } break;

    default: {
      result = DefWindowProcA(windowHandle, messageID, wParam, lParam);
    } break;
  }

  return result;
}

int WINAPI WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE previousInstance,
                   _In_ LPSTR commandLine, _In_ int commandShow) {
  Win32ResizeDIBSection(&gameBackBuffer, 1280, 720);

  WNDCLASSA windowClass = {
      .style = CS_HREDRAW | CS_VREDRAW,  // redraw window when size changes
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

  HWND windowHandle =
      CreateWindowExA(0,                          // extended window style
                      windowClass.lpszClassName,  // window class name
                      "Handmade Hero",            // window name
                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,  // window style
                      CW_USEDEFAULT,                     // x
                      CW_USEDEFAULT,                     // y
                      CW_USEDEFAULT,                     // width
                      CW_USEDEFAULT,                     // height
                      0,                                 // parent window handle
                      0,                                 // menu handle
                      instance,  // module's instance handle
                      0          // pointer to value of WM_CREATE
      );
  if (windowHandle == NULL) {
    OutputDebugStringA("Error:CreateWindowExA\n");
    return 0;
  }

  MSG msg = {0};
  running = TRUE;

  while (running) {
    while (PeekMessageA(&msg, windowHandle, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        running = FALSE;
      }
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }

    renderStuff(gameBackBuffer);

    HDC deviceContext = GetDC(windowHandle);
    Win32WindowDimensions windowDims = Win32GetWindowDimension(windowHandle);
    Win32CopyBufferToWindow(deviceContext, windowDims.width, windowDims.height,
                            gameBackBuffer);
    ReleaseDC(windowHandle, deviceContext);
  }

  // MessageBoxA(NULL, "This is Handmade Hero", "Handmade Hero",
  //            MB_OK | MB_ICONINFORMATION);

  return (int)msg.wParam;

  UNREFERENCED_PARAMETER(previousInstance);
  UNREFERENCED_PARAMETER(commandLine);
  UNREFERENCED_PARAMETER(commandShow);
}
