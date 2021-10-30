#pragma once

#include "Constants.hh"



class AudioCapture {
public:
  AudioCapture(const char* device_name, int sample_rate, int format,
      int buffer_size);
  ~AudioCapture();

  // Gets up to frame_count frames from the buffer, returning the number of
  // frames actually read. If wait is true, this function does not return until
  // the requested number of frames has actually been read from the audio
  // device, even if it has to wait for more frames to be recorded.
  size_t get_samples(void* buffer, size_t sample_count, bool wait = false);
  size_t get_frames(void* buffer, size_t frame_count, bool wait = false);

private:
  ALCdevice* device;
  int format;
};
