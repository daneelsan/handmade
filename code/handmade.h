#if !defined(HANDMADE_H)

#define internal static
#define global_variable static
#define local_persist static

#define PI_32 3.14159265359f

typedef float f32;
typedef double f64;

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef i32 b32;

/*
  TODO(casey): Services that the plaform layer provides to the game
*/

/*
  NOTE(casey): Services that the game provides to the platform layer
*/

typedef struct GameOffscreenBuffer {
  void *memory;
  int width;
  int height;
  int pitch;
} GameOffscreenBuffer;

typedef struct GameSoundBuffer {
  int samplesPerSec;
  int outputFramesCount;
  i16 *samples;
} GameSoundBuffer;

// Arguments: timing, input, sound buffer to use, bitmap buffer to use
internal void GameUpdateAndRender(
    GameOffscreenBuffer *screen, int blueOffset, int greenOffset, GameSoundBuffer *sound, f32 waveFrequency);

#define HANDMADE_H
#endif
