#include "handmade.h"

// TODO(casey): Implement sinf ourselves
#include "math.h"  // sinf

typedef struct GameState {
  int toneHz;
  int blueOffset;
  int greenOffset;
} GameState;

// TODO(casey): Allow sample offsets here for more robust platform options
internal void GameOutputSound(GameSoundBuffer *sound, int waveFrequency) {
  local_persist f32 tSine;
  int toneVolume = 3000;
  f32 wavePeriod = sound->samplesPerSec / (f32)waveFrequency;

  i16 *samples = sound->samples;
  for (int frameIndex = 0; frameIndex < sound->outputFramesCount; ++frameIndex) {
    f32 sineValue = sinf(tSine);
    i16 sampleValue = (i16)(sineValue * toneVolume);
    *samples++ = sampleValue;
    *samples++ = sampleValue;

    tSine += 2.0f * PI_32 / wavePeriod;
  }
}

internal void RenderWeirdGradient(GameOffscreenBuffer *screen, int blueOffset, int greenOffset) {
  // TODO: See what optimizer does in value vs reference
  u8 *row = (u8 *)screen->memory;

  for (int y = 0; y < screen->height; ++y) {
    u32 *pixel = (u32 *)row;
    for (int x = 0; x < screen->width; ++x) {
      u8 green = (u8)(x + greenOffset);
      u8 blue = (u8)(y + blueOffset);

      *pixel = (green << 8) + blue;
      pixel += 1;
    }
    row += screen->pitch;
  }
}

internal void GameUpdateAndRender(GameMemory *memory,
                                  GameOffscreenBuffer *screen,
                                  GameSoundBuffer *sound,
                                  GameInput *input) {
  ASSERT(sizeof(GameState) <= memory->permanent.size);

  GameState *gameState = (GameState *)memory->permanent.storage;
  if (!memory->isInitialized) {
    gameState->toneHz = 256;
    gameState->blueOffset = 0;
    gameState->greenOffset = 0;

    // NOTE(casey): This may be more appropriate to do in the platform layer
    memory->isInitialized = TRUE;
  }

  GameControllerInput *input0 = &input->controllers[0];
  if (input0->isAnalog) {
    // NOTE(casey): Use analog movement tuning
    gameState->blueOffset += (int)(4.0f * input0->x.end);
    gameState->toneHz = 256 + (int)(128.0f * input0->y.end);
  } else {
    // NOTE(casey): Use digital movement tuning
  }

  if (input0->button.down.endedDown) {
    gameState->greenOffset += 1;
  }

  // TODO(casey): Allow sample offsets here for more robust platform options
  GameOutputSound(sound, gameState->toneHz);
  RenderWeirdGradient(screen, gameState->blueOffset, gameState->greenOffset);
}
