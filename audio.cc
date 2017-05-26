#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <phosg/Filesystem.hh>
#include <phosg/Strings.hh>
#include <stdexcept>

#include "audio.hh"

using namespace std;


const char* al_err_str(ALenum err) {
  switch(err) {
    case AL_NO_ERROR:
      return "AL_NO_ERROR";
    case AL_INVALID_NAME:
      return "AL_INVALID_NAME";
    case AL_INVALID_ENUM:
      return "AL_INVALID_ENUM";
    case AL_INVALID_VALUE:
      return "AL_INVALID_VALUE";
    case AL_INVALID_OPERATION:
      return "AL_INVALID_OPERATION";
    case AL_OUT_OF_MEMORY:
      return "AL_OUT_OF_MEMORY";
  }
  return "unknown";
}

#define __al_check_error(file,line) \
  do { \
    for (ALenum err = alGetError(); err != AL_NO_ERROR; err = alGetError()) \
      fprintf(stderr, "AL error %s at %s:%d\n", al_err_str(err), file, line); \
  } while(0)

#define al_check_error() \
    __al_check_error(__FILE__, __LINE__)


void init_al() {
  const char *defname = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);

  ALCdevice* dev = alcOpenDevice(defname);
  ALCcontext* ctx = alcCreateContext(dev, NULL);
  alcMakeContextCurrent(ctx);
}

void exit_al() {
  ALCcontext* ctx = alcGetCurrentContext();
  ALCdevice* dev = alcGetContextsDevice(ctx);

  alcMakeContextCurrent(NULL);
  alcDestroyContext(ctx);
  alcCloseDevice(dev);
}



Sound::Sound() : seconds(0), sample_rate(0), buffer_id(0), source_id(0),
    num_samples(0), samples8(NULL) { }

Sound::Sound(float seconds, uint32_t sample_rate, bool is_16bit) :
    seconds(seconds), sample_rate(sample_rate), is_16bit(is_16bit) {
  this->num_samples = this->seconds * this->sample_rate;
  if (is_16bit) {
    this->samples16 = new int16_t[this->num_samples];
  } else {
    this->samples8 = new int8_t[this->num_samples];
  }
}

Sound::~Sound() {
  // unbind buffer from source
  if (this->source_id) {
    alDeleteSources(1, &this->source_id);
  }
  if (this->buffer_id) {
    alDeleteBuffers(1, &this->buffer_id);
  }
  if (is_16bit) {
    if (this->samples16) {
      delete[] this->samples16;
    }
  } else {
    if (this->samples8) {
      delete[] this->samples8;
    }
  }
}

void Sound::create_al_objects() {
  alGenBuffers(1, &this->buffer_id);
  al_check_error();

  if (this->is_16bit) {
    alBufferData(this->buffer_id, AL_FORMAT_MONO16, this->samples16,
        this->num_samples * sizeof(int16_t), this->sample_rate);
  } else {
    alBufferData(this->buffer_id, AL_FORMAT_MONO8, this->samples8,
        this->num_samples * sizeof(int8_t), this->sample_rate);
  }
  al_check_error();

  alGenSources(1, &this->source_id);
  alSourcei(this->source_id, AL_BUFFER, this->buffer_id);
  al_check_error();
}

void Sound::play() {
  alSourcePlay(this->source_id);
}



SampledSound::SampledSound(const char* filename) {
  auto f = fopen_unique(filename, "rb");
  this->load(f.get());
}

SampledSound::SampledSound(FILE* f) {
  this->load(f);
}

struct RIFFHeader {
  uint32_t riff_magic;   // 0x52494646 big-endian ('RIFF')
  uint32_t file_size;    // size of file - 8
};

struct WAVHeader {
  uint32_t wave_magic;   // 0x57415645 big-endian ('WAVE')

  uint32_t fmt_magic;    // 0x666d7420
  uint32_t fmt_size;     // 16
  uint16_t format;       // 1 = PCM
  uint16_t num_channels;
  uint32_t sample_rate;
  uint32_t byte_rate;    // num_channels * sample_rate * bits_per_sample / 8
  uint16_t block_align;  // num_channels * bits_per_sample / 8
  uint16_t bits_per_sample;

  uint32_t data_magic;   // 0x64617461
  uint32_t data_size;    // num_samples * num_channels * bits_per_sample / 8
};

void SampledSound::load(FILE* f) {
  // check the RIFF header
  uint32_t magic;
  freadx(f, &magic, sizeof(uint32_t));
  if (magic != 0x46464952) {
    throw runtime_error(string_printf("unknown file format: %08" PRIX32, magic));
  }

  uint32_t file_size;
  freadx(f, &file_size, sizeof(uint32_t));

  // load the WAV header
  WAVHeader wav;
  freadx(f, &wav, sizeof(WAVHeader));

  // check the header info. we only support 1-channel, 16-bit sounds for now
  if (wav.wave_magic != 0x45564157) { // 'WAVE'
    throw runtime_error(string_printf("sound has incorrect wave_magic (%" PRIX32 ")", wav.wave_magic));
  }
  if (wav.fmt_magic != 0x20746D66) { // 'fmt '
    throw runtime_error(string_printf("sound has incorrect fmt_magic (%" PRIX32 ")", wav.fmt_magic));
  }
  if (wav.data_magic != 0x61746164) { // 'data'
    throw runtime_error(string_printf("sound has incorrect data_magic (%" PRIX32 ")", wav.data_magic));
  }
  if (wav.num_channels != 1) {
    throw runtime_error(string_printf("sound has too many channels (%" PRIu16 ")", wav.num_channels));
  }
  if ((wav.bits_per_sample != 8) && (wav.bits_per_sample != 16)) {
    throw runtime_error(string_printf("sample bit width is unsupported (%" PRIu16 ")", wav.bits_per_sample));
  }
  if (wav.format != 1) {
    throw runtime_error(string_printf("format is not PCM (%" PRIu16 ")", wav.format));
  }

  // TODO: if we support multi-channel sounds in the future, we'll have to deal
  // with block_align here
  this->sample_rate = wav.sample_rate;
  this->num_samples = wav.data_size / (wav.bits_per_sample / 8);
  this->seconds = static_cast<float>(this->num_samples) / this->sample_rate;
  this->is_16bit = (wav.bits_per_sample == 16);

  // load the samples
  if (this->is_16bit) {
    this->samples16 = new int16_t[this->num_samples];
    freadx(f, this->samples16, this->num_samples * sizeof(int16_t));
  } else {
    this->samples8 = new int8_t[this->num_samples];
    freadx(f, this->samples8, this->num_samples * sizeof(int8_t));
  }

  // load it into audio memory
  this->create_al_objects();
}



// all GeneratedSounds are 16-bit (hence the `true`)
GeneratedSound::GeneratedSound(float seconds, uint32_t sample_rate) :
    Sound(seconds, sample_rate, true) { }



SineWave::SineWave(float frequency, float seconds, float volume,
    uint32_t sample_rate) : GeneratedSound(seconds, sample_rate),
    frequency(frequency), volume(volume) {
  for (size_t x = 0; x < this->num_samples; x++) {
    this->samples16[x] = 32760 * sin((2.0f * M_PI * this->frequency) / this->sample_rate * x) * this->volume;
  }
  this->create_al_objects();
}

SquareWave::SquareWave(float frequency, float seconds, float volume,
    uint32_t sample_rate) : GeneratedSound(seconds, sample_rate),
    frequency(frequency), volume(volume) {
  for (size_t x = 0; x < this->num_samples; x++) {
    if ((uint64_t)((2 * this->frequency) / this->sample_rate * x) & 1) {
      this->samples16[x] = 32760 * this->volume;
    } else {
      this->samples16[x] = -32760 * this->volume;
    }
  }
  this->create_al_objects();
}

WhiteNoise::WhiteNoise(float seconds, float volume, uint32_t sample_rate) :
    GeneratedSound(seconds, sample_rate), volume(volume) {
  for (size_t x = 0; x < this->num_samples; x++) {
    this->samples16[x] = rand() - RAND_MAX / 2;
  }
  this->create_al_objects();
}

SplitNoise::SplitNoise(int split_distance, float seconds, float volume,
    bool fade_out, uint32_t sample_rate) : GeneratedSound(seconds, sample_rate),
    split_distance(split_distance), volume(volume) {

  for (size_t x = 0; x < this->num_samples; x += split_distance) {
    this->samples16[x] = rand() - RAND_MAX / 2;
  }

  for (size_t x = 0; x < this->num_samples; x++) {
    if ((x % split_distance) == 0)
      continue;

    int first_x = (x / split_distance) * split_distance;
    int second_x = first_x + split_distance;
    float x1p = (float)(x - first_x) / (second_x - first_x);
    if (second_x >= this->num_samples)
      this->samples16[x] = 0;
    else
      this->samples16[x] = x1p * this->samples16[first_x] + (1 - x1p) * this->samples16[second_x];
  }

  // TODO generalize
  if (fade_out) {
    for (size_t x = 0; x < this->num_samples; x++) {
      this->samples16[x] *= (float)(this->num_samples - x) / this->num_samples;
    }
  }
  this->create_al_objects();
}
