#pragma warning(push, 3)
#include <windows.h>
#pragma warning(pop)

#define internal static
#define global_variable static
#define local_persist static

// @NOTE: `running` is global for now
global_variable BOOL running;
global_variable BITMAPINFO bitmapInfo;
global_variable VOID *bitmapMemory;
global_variable HBITMAP bitmapHandle;
global_variable HDC bitmapDeviceContext;

LRESULT CALLBACK Win32MainWindowCallback(_In_ HWND windowHandle,
                                         _In_ UINT messageID,
                                         _In_ WPARAM wParam,
                                         _In_ LPARAM lParam);
internal void Win32ResizeDIBSection(int width, int height);
internal void Win32UpdateWindow(HDC deviceContext, int x, int y, int width,
                                int height);

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
  BOOL msgResult;
  running = TRUE;

  while (running) {
    msgResult = GetMessageA(&msg, NULL, 0, 0);
    if (msgResult == -1) {
      OutputDebugStringA("Error:GetMessageA\n");
      return 0;
    } else {
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }
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

      int x = paint.rcPaint.left;
      int y = paint.rcPaint.top;
      int width = paint.rcPaint.right - x;
      int height = paint.rcPaint.bottom - y;
      Win32UpdateWindow(deviceContext, x, y, width, height);

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
  // @TODO: Bullet proof this
  // Maybe don't free first, free after, then free first if that fails

  if (bitmapHandle != NULL) {
    DeleteObject(bitmapHandle);
  }
  
  if (bitmapDeviceContext == NULL) {
    bitmapDeviceContext = CreateCompatibleDC(NULL);
  }

  bitmapInfo = (BITMAPINFO){
      .bmiHeader =
          {
              .biSize = sizeof(BITMAPINFOHEADER),
              .biWidth = width,
              .biHeight = height,
              .biPlanes = 1,
              .biBitCount = 32,
              .biCompression = BI_RGB,
          },
  };
  bitmapHandle = CreateDIBSection(bitmapDeviceContext, &bitmapInfo,
                                  DIB_RGB_COLORS, &bitmapMemory, NULL, 0);
}

internal void Win32UpdateWindow(HDC deviceContext, int x, int y, int width,
                                int height) {
  StretchDIBits(deviceContext, x, y, width, height, x, y, width, height,
                bitmapMemory, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}
