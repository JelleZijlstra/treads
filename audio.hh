#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef MACOSX
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

void init_al();
void exit_al();

class Sound {
public:
  virtual ~Sound();

  void play();

protected:
  Sound();
  Sound(float seconds, uint32_t sample_rate = 44100, bool is_16bit = true);

  // these are deleted because I'm lazy
  Sound(const Sound&) = delete;
  Sound(Sound&&) = delete;
  Sound& operator=(const Sound&) = delete;
  Sound& operator=(Sound&&) = delete;

  void create_al_objects();

  float seconds;
  uint32_t sample_rate;

  ALuint buffer_id;
  ALuint source_id;

  bool is_16bit;
  size_t num_samples;
  union {
    int16_t* samples16;
    int8_t* samples8;
  };
};

class SampledSound : public Sound {
public:
  SampledSound(const char* filename);
  SampledSound(FILE* f);
  virtual ~SampledSound() = default;

protected:
  void load(FILE* f);
};

class GeneratedSound : public Sound {
public:
  virtual ~GeneratedSound() = default;

protected:
  GeneratedSound(float seconds, uint32_t sample_rate = 44100);
};

class SineWave : public GeneratedSound {
public:
  SineWave(float frequency, float seconds, float volume = 1.0,
      uint32_t sample_rate = 44100);

protected:
  float frequency;
  float volume;
};

class SquareWave : public GeneratedSound {
public:
  SquareWave(float frequency, float seconds, float volume = 1.0,
      uint32_t sample_rate = 44100);

protected:
  float frequency;
  float volume;
};

class WhiteNoise : public GeneratedSound {
public:
  WhiteNoise(float seconds, float volume = 1.0, uint32_t sample_rate = 44100);

protected:
  float volume;
};

class SplitNoise : public GeneratedSound {
public:
  SplitNoise(int split_distance, float seconds, float volume = 1.0,
      bool fade_out = false, uint32_t sample_rate = 44100);

protected:
  int split_distance;
  float volume;
};
