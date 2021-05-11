#if !defined(HANDMADE_H)

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

// Arguments: timing, input, sound buffer to use, bitmap buffer to use
internal void GameUpdateAndRender(GameOffscreenBuffer *buffer, int blueOffset, int greenOffset);

#define HANDMADE_H
#endif
