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
global_variable BITMAPINFO bitmapInfo;
global_variable VOID *bitmapMemory;
global_variable WORD bytesPerPixel = 4;
global_variable int bitmapWidth;
global_variable int bitmapHeight;

internal void renderStuff(void) {
  local_persist int offset = 0;

  int width = bitmapWidth;
  int height = bitmapHeight;
  int pitch = width * bytesPerPixel;
  u8 *row = (u8 *)bitmapMemory;

  for (int y = 0; y < height; ++y) {
    u32 *pixel = (u32 *)row;
    for (int x = 0; x < width; ++x) {
      *pixel = (u32)(x + y + offset);
      pixel += 1;
    }
    row += pitch;
  }

  ++offset;
}


LRESULT CALLBACK Win32MainWindowCallback(_In_ HWND windowHandle,
                                         _In_ UINT messageID,
                                         _In_ WPARAM wParam,
                                         _In_ LPARAM lParam);
internal void Win32ResizeDIBSection(int width, int height);
internal void Win32UpdateWindow(HDC deviceContext, RECT windowRect);

int WINAPI WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE previousInstance,
                   _In_ LPSTR commandLine, _In_ int commandShow) {
  WNDCLASSA windowClass = {
      .lpfnWndProc =
          Win32MainWindowCallback,  // pointer to the window procedure
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

    renderStuff();

    HDC deviceContext = GetDC(windowHandle);
    RECT clientRect;
    GetClientRect(windowHandle, &clientRect);
    Win32UpdateWindow(deviceContext, clientRect);
    ReleaseDC(windowHandle, deviceContext);
  }

  // MessageBoxA(NULL, "This is Handmade Hero", "Handmade Hero",
  //            MB_OK | MB_ICONINFORMATION);

  return 0;

  UNREFERENCED_PARAMETER(previousInstance);
  UNREFERENCED_PARAMETER(commandLine);
  UNREFERENCED_PARAMETER(commandShow);
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

      /*int x = paint.rcPaint.left;
      int y = paint.rcPaint.top;
      int width = paint.rcPaint.right - x;
      int height = paint.rcPaint.bottom - y;*/

      RECT clientRect;
      GetClientRect(windowHandle, &clientRect);
      Win32UpdateWindow(deviceContext, clientRect);

      TextOutA(deviceContext, 0, 0, "Hello, Windows!", 15);

      EndPaint(windowHandle, &paint);
    } break;

    case WM_SIZE: {
      RECT clientRect;
      GetClientRect(windowHandle, &clientRect);
      int width = clientRect.right - clientRect.left;
      int height = clientRect.bottom - clientRect.top;
      Win32ResizeDIBSection(width, height);
    } break;

    default: {
      result = DefWindowProcA(windowHandle, messageID, wParam, lParam);
    } break;
  }

  return result;
}
internal void Win32ResizeDIBSection(int width, int height) {
  if (bitmapMemory != NULL) {
    VirtualFree(bitmapMemory, 0, MEM_RELEASE);
  }

  bitmapWidth = width;
  bitmapHeight = height;

  bitmapInfo = (BITMAPINFO){
      .bmiHeader =
          {
              .biSize = sizeof(BITMAPINFOHEADER),
              .biWidth = width,
              .biHeight = -height,  // top-down image
              .biPlanes = 1,
              .biBitCount = bytesPerPixel * 8,
              .biCompression = BI_RGB,
          },
  };

  SIZE_T bitmapMemorySize = ((SIZE_T)width * (SIZE_T)height * bytesPerPixel);
  bitmapMemory =
      VirtualAlloc(NULL, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
  if (bitmapMemory == NULL) {
    OutputDebugStringA("Error:VirtualAlloc");
    return;
  }

  // renderStuff();
}

internal void Win32UpdateWindow(HDC deviceContext, RECT windowRect) {
  LONG windowWidth = windowRect.right - windowRect.left;
  LONG windowHeight = windowRect.bottom - windowRect.top;
  StretchDIBits(deviceContext,
                // destination rectangle
                0, 0, bitmapWidth, bitmapHeight,
                // source rectangle
                0, 0, windowWidth, windowHeight, bitmapMemory, &bitmapInfo,
                DIB_RGB_COLORS, SRCCOPY);
}
