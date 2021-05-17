#if !defined(HANDMADE_H)

#define internal static
#define global_variable static
#define local_persist static

#define PI_32 3.14159265359f
#define TRUE 1
#define FALSE 0

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
  NOTE(casey):

  HANDMADE_INTERNAL:
    0 - Build for public release
    1 - Build for developer only
  HANDMADE_SLOW:
    0 - Gotta go fast!
    1 - Turtle code
*/

// TODO(casey): Complete assertion macro
#if HANDMADE_SLOW
#define ASSERT(expr) \
  if (!(expr)) {     \
    *(int *)0 = 0;   \
  }
#else
#define ASSERT(expr)
#endif

#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

#define KYLOBYTE 1024LL
#define KILOBYTES(n) ((n)*KYLOBYTE)
#define MEGABYTES(n) (KILOBYTES(n) * KYLOBYTE)
#define GIGABYTES(n) (MEGABYTES(n) * KYLOBYTE)
#define TERABYTES(n) (GIGABYTES(n) * KYLOBYTE)

/*
  TODO(casey): Services that the plaform layer provides to the game
*/

/*
  NOTE(casey): Services that the game provides to the platform layer
*/

/* Video */

typedef struct GameOffscreenBuffer {
  void *memory;
  int width;
  int height;
  int pitch;
} GameOffscreenBuffer;

/* Sound */

typedef struct GameSoundBuffer {
  int samplesPerSec;
  int outputFramesCount;
  i16 *samples;
} GameSoundBuffer;

/* Input */

typedef struct GameAnalogStickState {
  f32 start;
  f32 min;
  f32 max;
  f32 end;
} GameAnalogStickState;

typedef struct GameButtonState {
  int halfTransitionCount;
  b32 endedDown;
} GameButtonState;

typedef struct GameControllerInput {
  b32 isAnalog;

  GameAnalogStickState x;
  GameAnalogStickState y;

  union {
    GameButtonState buttons[6];
    struct {
      GameButtonState up;
      GameButtonState down;
      GameButtonState left;
      GameButtonState right;
      GameButtonState leftShoulder;
      GameButtonState rightShoulder;
    } button;
  };
} GameControllerInput;

typedef struct GameInput {
  // TODO(casey): Insert clock here
  GameControllerInput controllers[4];
} GameInput;

/* Memory */

struct GameMemoryStorage {
  void *storage;
  u64 count;
  u64 size;
};

// NOTE(daniel): Storages are REQUIRED to be cleared to zero at startup
typedef struct GameMemory {
  struct GameMemoryStorage permanent;
  struct GameMemoryStorage transient;
  b32 isInitialized;
} GameMemory;

/* Main entry */

// Arguments: timing, input, sound buffer to use, bitmap buffer to use
internal void GameUpdateAndRender(GameMemory *memory,
                                  GameOffscreenBuffer *screen,
                                  GameSoundBuffer *sound,
                                  GameInput *input);

#define HANDMADE_H
#endif
