#include "handmade.h"

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

internal void GameUpdateAndRender(GameOffscreenBuffer *screen, GameSoundBuffer *sound, GameInput *input) {
  local_persist int blueOffset = 0;
  local_persist int greenOffset = 0;
  local_persist int toneHz = 256;

  GameControllerInput *input0 = &input->controllers[0];
  if (input0->isAnalog) {
    // NOTE(casey): Use analog movement tuning
    blueOffset += (int)(4.0f * input0->x.end);
    toneHz += (int)(128.0f * input0->y.end);
  } else {
    // NOTE(casey): Use digital movement tuning
  }

  if (input0->button.down.endedDown) {
    greenOffset += 1;
  }

  // TODO(casey): Allow sample offsets here for more robust platform options
  GameOutputSound(sound, toneHz);
  RenderWeirdGradient(screen, blueOffset, greenOffset);
}
