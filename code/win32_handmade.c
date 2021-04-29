#pragma warning(push, 3)
#include <windows.h>
#pragma warning(pop)

LRESULT CALLBACK MainWindowCallback(_In_ HWND windowHandle, _In_ UINT messageID,
                                    _In_ WPARAM wParam, _In_ LPARAM lParam);

int WINAPI WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE previousInstance,
                   _In_ LPSTR commandLine, _In_ int commandShow) {
  WNDCLASSA windowClass = {
      // @TODO: Check if these styles still matter
      .style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
      .lpfnWndProc = MainWindowCallback,  // pointer to the window procedure
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

  while ((msgResult = GetMessageA(&msg, NULL, 0, 0)) != 0) {
    if (msgResult == -1) {
      OutputDebugStringA("Error:GetMessageA\n");
      return 0;
    } else {
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }
  }

  MessageBoxA(NULL, "This is Handmade Hero", "Handmade Hero",
              MB_OK | MB_ICONINFORMATION);

  return 0;

  UNREFERENCED_PARAMETER(previousInstance);
  UNREFERENCED_PARAMETER(commandLine);
  UNREFERENCED_PARAMETER(commandShow);
}

LRESULT CALLBACK MainWindowCallback(_In_ HWND windowHandle, _In_ UINT messageID,
                                    _In_ WPARAM wParam, _In_ LPARAM lParam) {
  LRESULT result = 0;

  switch (messageID) {
    case WM_ACTIVATEAPP: {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    } break;

    case WM_CLOSE: {
      // if (MessageBox(windowHandle, "Close?", "Game_B", MB_YESNOCANCEL) ==
      //    IDYES) {
      //  DestroyWindow(windowHandle);
      //}
      OutputDebugStringA("WM_CLOSE\n");
      DestroyWindow(windowHandle);
    } break;

    case WM_CREATE: {
      OutputDebugStringA("WM_CREATE\n");
    } break;

    case WM_DESTROY: {
      // Clean up window-specific data objects
      OutputDebugStringA("WM_DESTROY\n");
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
      static DWORD rop = WHITENESS;
      rop = (rop == WHITENESS) ? BLACKNESS : WHITENESS;
      PatBlt(deviceContext, x, y, width, height, rop);

      TextOutA(deviceContext, 0, 0, "Hello, Windows!", 15);

      EndPaint(windowHandle, &paint);
    } break;

    case WM_SIZE: {
      OutputDebugStringA("WM_SIZE\n");
    } break;

    default: {
      result = DefWindowProcA(windowHandle, messageID, wParam, lParam);
    } break;
  }

  return result;
}
