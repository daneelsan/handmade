#ifndef WIN32_HANDMADE_H
#define WIN32_HANDMADE_H

typedef struct Win32OffscreenBuffer {
  void *memory;
  BITMAPINFO info;
  int width;
  int height;
  int bytesPerPixel;
  int pitch;
} Win32OffscreenBuffer;

typedef struct Win32WindowDimensions {
  int width;
  int height;
} Win32WindowDimensions;

// TODO(daniel): Fix names involving frames and samples. What is the difference?
typedef struct Win32SoundOutput {
  WORD nChannels;
  WORD bytesPerSample;
  WORD bytesPerFrame;
  DWORD samplesPerSec;
  f32 tSine;
  u16 toneVolume;
  int framesLatency;
  u32 runningFrameIndex;
  DWORD bufferSize;
  LPDIRECTSOUNDBUFFER buffer;
} Win32SoundOutput;

#endif  // WIN32_HANDMADE_H
