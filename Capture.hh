#pragma once

#include "Constants.hh"



class AudioCapture {
public:
  AudioCapture(const char* device_name, int sample_rate, int format,
      int buffer_size);
  ~AudioCapture();

  // Gets up to sample_count samples from the buffer, returning the number of
  // samples actually read. If wait is true, this function does not return until
  // the requested number of samples has actually been read from the audio
  // device, even if it has to wait for more samples to be recorded.
  size_t get_samples(void* buffer, size_t sample_count, bool wait = false);

private:
  ALCdevice* device;
  int format;
};
