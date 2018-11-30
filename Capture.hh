#pragma once

#include "Constants.hh"



class AudioCapture {
public:
  AudioCapture(const char* device_name, int sample_rate, int format,
      int buffer_size);
  ~AudioCapture();

  size_t get_samples(void* buffer, size_t sample_count);

private:
  ALCdevice* device;
};
