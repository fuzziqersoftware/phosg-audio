#include "Sound.hh"

#include <inttypes.h>
#include <math.h>

#include <phosg/Filesystem.hh>
#include <phosg/Strings.hh>
#include <stdexcept>

#include "File.hh"

using namespace std;

namespace phosg_audio {

Sound::Sound(uint32_t sample_rate)
    : buffer_id(0), source_id(0), sample_rate(sample_rate) {}

Sound::~Sound() {
  if (this->source_id) {
    alDeleteSources(1, &this->source_id);
  }
  if (this->buffer_id) {
    alDeleteBuffers(1, &this->buffer_id);
  }
}

void Sound::print(FILE* stream) const {
  for (size_t x = 0; x < this->samples.size(); x++) {
    fprintf(stream, "%zu: %g\n", x, this->samples[x]);
  }
}

void Sound::write(FILE* stream) const {
  fwrite(this->samples.data(), sizeof(this->samples[0]), this->samples.size(),
      stream);
}

void Sound::play() {
  alSourcePlay(this->source_id);
}

void Sound::set_volume(float volume) {
  alSourcef(this->source_id, AL_GAIN, volume);
}

void Sound::create_al_objects() {
  alGenBuffers(1, &this->buffer_id);
  al_check_error();

  // Windows OpenAL doesn't support float32 format, so use int16 instead
#ifdef WINDOWS
  auto int_samples = convert_samples_to_int(this->samples);
  alBufferData(this->buffer_id, AL_FORMAT_MONO16, int_samples.data(),
      int_samples.size() * sizeof(int16_t), this->sample_rate);
#else
  alBufferData(this->buffer_id, alGetEnumValue("AL_FORMAT_MONO_FLOAT32"),
      this->samples.data(), this->samples.size() * sizeof(float),
      this->sample_rate);
#endif
  al_check_error();

  alGenSources(1, &this->source_id);
  alSourcei(this->source_id, AL_BUFFER, this->buffer_id);
  al_check_error();
}

SampledSound::SampledSound(const char* filename) : Sound(0) {
  auto wav = load_wav(filename);
  this->sample_rate = wav.sample_rate;
  this->samples = std::move(wav.samples);
  this->create_al_objects();
}

SampledSound::SampledSound(const string& filename) : SampledSound(filename.c_str()) {}

SampledSound::SampledSound(FILE* f) : Sound(0) {
  auto wav = load_wav(f);
  this->sample_rate = wav.sample_rate;
  this->samples = std::move(wav.samples);
  this->create_al_objects();
}

GeneratedSound::GeneratedSound(float seconds, float volume,
    uint32_t sample_rate) : Sound(sample_rate), seconds(seconds), volume(volume) {
  this->samples.resize(this->seconds * this->sample_rate);
}

SineWave::SineWave(float frequency, float seconds, float volume,
    uint32_t sample_rate) : GeneratedSound(seconds, volume, sample_rate),
                            frequency(frequency) {
  for (size_t x = 0; x < this->samples.size(); x++) {
    this->samples[x] = sin((6.283185307179586 * this->frequency) / this->sample_rate * x) * this->volume;
  }
  this->create_al_objects();
}

SquareWave::SquareWave(float frequency, float seconds, float volume,
    uint32_t sample_rate) : GeneratedSound(seconds, volume, sample_rate),
                            frequency(frequency) {
  for (size_t x = 0; x < this->samples.size(); x++) {
    if ((uint64_t)((2 * this->frequency) / this->sample_rate * x) & 1) {
      this->samples[x] = this->volume;
    } else {
      this->samples[x] = -this->volume;
    }
  }
  this->create_al_objects();
}

TriangleWave::TriangleWave(float frequency, float seconds, float volume,
    uint32_t sample_rate) : GeneratedSound(seconds, volume, sample_rate),
                            frequency(frequency) {
  size_t period_length = this->sample_rate / (2 * this->frequency);
  for (size_t x = 0; x < this->samples.size(); x++) {
    size_t period_index = x / period_length;
    double factor = (double)(x % period_length) / period_length;
    if (period_index & 1) {
      // Down-slope; linear from 1 -> -1
      this->samples[x] = (1 - 2 * factor) * this->volume;
    } else {
      // Up-slope; linear from -1 -> 1
      this->samples[x] = (-1 + 2 * factor) * this->volume;
    }
  }
  this->create_al_objects();
}

FrontTriangleWave::FrontTriangleWave(float frequency, float seconds,
    float volume, uint32_t sample_rate) : GeneratedSound(seconds, volume, sample_rate), frequency(frequency) {
  size_t period_length = this->sample_rate / this->frequency;
  for (size_t x = 0; x < this->samples.size(); x++) {
    double factor = (double)(x % period_length) / period_length;
    // Up-slope; linear from -1 -> 1
    this->samples[x] = (-1 + 2 * factor) * this->volume;
  }
  this->create_al_objects();
}

WhiteNoise::WhiteNoise(float seconds, float volume, uint32_t sample_rate) : GeneratedSound(seconds, volume, sample_rate) {
  for (size_t x = 0; x < this->samples.size(); x++) {
    this->samples[x] = (static_cast<float>(rand() * 2) / RAND_MAX) - 1.0;
  }
  this->create_al_objects();
}

SplitNoise::SplitNoise(int split_distance, float seconds, float volume,
    bool fade_out, uint32_t sample_rate) : GeneratedSound(seconds, volume, sample_rate),
                                           split_distance(split_distance) {

  for (size_t x = 0; x < this->samples.size(); x += split_distance) {
    this->samples[x] = (static_cast<float>(rand() * 2) / RAND_MAX) - 1.0;
  }

  for (size_t x = 0; x < this->samples.size(); x++) {
    if ((x % split_distance) == 0) {
      continue;
    }

    size_t first_x = (x / split_distance) * split_distance;
    size_t second_x = first_x + split_distance;
    float x1p = (float)(x - first_x) / (second_x - first_x);
    if (second_x >= this->samples.size()) {
      this->samples[x] = 0;
    } else {
      this->samples[x] = x1p * this->samples[first_x] + (1 - x1p) * this->samples[second_x];
    }
  }

  // TODO: generalize this
  if (fade_out) {
    for (size_t x = 0; x < this->samples.size(); x++) {
      this->samples[x] *= (float)(this->samples.size() - x) / this->samples.size();
    }
  }
  this->create_al_objects();
}

} // namespace phosg_audio
