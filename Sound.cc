#include "Sound.hh"

#include <inttypes.h>

#include <phosg/Filesystem.hh>
#include <phosg/Strings.hh>
#include <stdexcept>

#include "File.hh"

using namespace std;



Sound::Sound(uint32_t sample_rate) : sample_rate(sample_rate), buffer_id(0),
    source_id(0) { }

Sound::~Sound() {
  if (this->source_id) {
    alDeleteSources(1, &this->source_id);
  }
  if (this->buffer_id) {
    alDeleteBuffers(1, &this->buffer_id);
  }
}

void Sound::play() {
  alSourcePlay(this->source_id);
}

void Sound::create_al_objects() {
  alGenBuffers(1, &this->buffer_id);
  al_check_error();

  alBufferData(this->buffer_id, alGetEnumValue("AL_FORMAT_MONO_FLOAT32"),
      this->samples.data(), this->samples.size() * sizeof(float),
      this->sample_rate);
  al_check_error();

  alGenSources(1, &this->source_id);
  alSourcei(this->source_id, AL_BUFFER, this->buffer_id);
  al_check_error();
}



SampledSound::SampledSound(const char* filename) : Sound(0) {
  auto wav = load_wav(filename);
  this->sample_rate = wav.sample_rate;
  this->samples = move(wav.samples);
  this->create_al_objects();
}

SampledSound::SampledSound(FILE* f) : Sound(0) {
  auto wav = load_wav(f);
  this->sample_rate = wav.sample_rate;
  this->samples = move(wav.samples);
  this->create_al_objects();
}



GeneratedSound::GeneratedSound(float seconds, float volume,
    uint32_t sample_rate) : Sound(sample_rate), seconds(seconds),
    volume(volume) {

  alGenBuffers(1, &this->buffer_id);
  al_check_error();

  this->samples.resize(this->seconds * this->sample_rate);
}

void GeneratedSound::fill_buffer_repeat(size_t num_samples_written) {
  for (size_t x = num_samples_written; x < this->samples.size(); x++) {
    this->samples[x] = this->samples[x % num_samples_written];
  }
}

void GeneratedSound::play() const {
  alSourcePlay(this->source_id);
}

void GeneratedSound::print(FILE* stream) const {
  for (size_t x = 0; x < this->samples.size(); x++) {
    fprintf(stream, "%zu: %g\n", x, this->samples[x]);
  }
}

void GeneratedSound::write(FILE* stream) const {
  fwrite(this->samples.data(), sizeof(this->samples[0]), this->samples.size(),
      stream);
}



SineWave::SineWave(float frequency, float seconds, float volume,
    uint32_t sample_rate) : GeneratedSound(seconds, volume, sample_rate),
    frequency(frequency) {
  for (size_t x = 0; x < this->samples.size(); x++) {
    this->samples[x] = sin((2.0f * M_PI * this->frequency) / this->sample_rate * x) * this->volume;
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
      // down-slope; linear from 1 -> -1
      this->samples[x] = (1 - 2 * factor) * this->volume;
    } else {
      // up-slope; linear from -1 -> 1
      this->samples[x] = (-1 + 2 * factor) * this->volume;
    }
  }
  this->create_al_objects();
}



FrontTriangleWave::FrontTriangleWave(float frequency, float seconds,
    float volume, uint32_t sample_rate) :
    GeneratedSound(seconds, volume, sample_rate), frequency(frequency) {
  size_t period_length = this->sample_rate / this->frequency;
  for (size_t x = 0; x < this->samples.size(); x++) {
    double factor = (double)(x % period_length) / period_length;
    // up-slope; linear from -1 -> 1
    this->samples[x] = (-1 + 2 * factor) * this->volume;
  }
  this->create_al_objects();
}



WhiteNoise::WhiteNoise(float seconds, float volume, uint32_t sample_rate) :
    GeneratedSound(seconds, volume, sample_rate) {
  for (size_t x = 0; x < this->samples.size(); x++) {
    this->samples[x] = (static_cast<float>(rand() * 2) / RAND_MAX) - 1.0;
  }
  this->create_al_objects();
}



SplitNoise::SplitNoise(int split_distance, float seconds, float volume,
    bool fade_out, uint32_t sample_rate) :
    GeneratedSound(seconds, volume, sample_rate),
    split_distance(split_distance) {

  for (size_t x = 0; x < this->samples.size(); x += split_distance) {
    this->samples[x] = (static_cast<float>(rand() * 2) / RAND_MAX) - 1.0;
  }

  for (size_t x = 0; x < this->samples.size(); x++) {
    if ((x % split_distance) == 0) {
      continue;
    }

    int first_x = (x / split_distance) * split_distance;
    int second_x = first_x + split_distance;
    float x1p = (float)(x - first_x) / (second_x - first_x);
    if (second_x >= this->samples.size()) {
      this->samples[x] = 0;
    } else {
      this->samples[x] = x1p * this->samples[first_x] + (1 - x1p) * this->samples[second_x];
    }
  }

  // TODO generalize
  if (fade_out) {
    for (size_t x = 0; x < this->samples.size(); x++) {
      this->samples[x] *= (float)(this->samples.size() - x) / this->samples.size();
    }
  }
  this->create_al_objects();
}
