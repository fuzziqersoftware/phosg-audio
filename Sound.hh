#pragma once

#include <string>

#include "Constants.hh"



class Sound {
public:
  virtual ~Sound();

  void print(FILE* stream) const;
  void write(FILE* stream) const;

  void play();
  void set_volume(float volume);

protected:
  explicit Sound(uint32_t sample_rate);

  // These are deleted because I'm lazy. Default copy/move constructors are bad
  // here because of the AL objects.
  Sound(const Sound&) = delete;
  Sound(Sound&&) = delete;
  Sound& operator=(const Sound&) = delete;
  Sound& operator=(Sound&&) = delete;

  void create_al_objects();

  ALuint buffer_id;
  ALuint source_id;

  uint32_t sample_rate;
  std::vector<float> samples;
};

class SampledSound : public Sound {
public:
  explicit SampledSound(const char* filename);
  explicit SampledSound(const std::string& filename);
  explicit SampledSound(FILE* f);
  virtual ~SampledSound() = default;
};

class GeneratedSound : public Sound {
public:
  virtual ~GeneratedSound() = default;

protected:
  explicit GeneratedSound(float seconds, float volume = 1.0,
      uint32_t sample_rate = 44100);

  float seconds;
  float volume;
};

class SineWave : public GeneratedSound {
public:
  SineWave(float frequency, float seconds, float volume = 1.0,
      uint32_t sample_rate = 44100);
  virtual ~SineWave() = default;

protected:
  float frequency;
};

class SquareWave : public GeneratedSound {
public:
  SquareWave(float frequency, float seconds, float volume = 1.0,
      uint32_t sample_rate = 44100);
  virtual ~SquareWave() = default;

protected:
  float frequency;
};

class TriangleWave : public GeneratedSound {
public:
  TriangleWave(float frequency, float seconds, float volume = 1.0,
      uint32_t sample_rate = 44100);
  virtual ~TriangleWave() = default;

protected:
  float frequency;
};

class FrontTriangleWave : public GeneratedSound {
public:
  FrontTriangleWave(float frequency, float seconds, float volume = 1.0,
      uint32_t sample_rate = 44100);
  virtual ~FrontTriangleWave() = default;

protected:
  float frequency;
};

class WhiteNoise : public GeneratedSound {
public:
  WhiteNoise(float seconds, float volume = 1.0, uint32_t sample_rate = 44100);
  virtual ~WhiteNoise() = default;
};

class SplitNoise : public GeneratedSound {
public:
  SplitNoise(int split_distance, float seconds, float volume = 1.0,
      bool fade_out = false, uint32_t sample_rate = 44100);
  virtual ~SplitNoise() = default;

protected:
  int split_distance;
};
