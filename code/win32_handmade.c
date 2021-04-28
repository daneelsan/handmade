#pragma warning(push, 3)
#include <windows.h>
#pragma warning(pop)

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                   _In_ LPSTR pCmdLine, _In_ int nCmdShow) {
  UNREFERENCED_PARAMETER(hInstance);
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(pCmdLine);
  UNREFERENCED_PARAMETER(nCmdShow);

  MessageBoxA(NULL, "This is Handmade Hero", "Handmade Hero",
              MB_OK | MB_ICONINFORMATION);

  return 0;
}
