#include "Capture.hh"

#include <unistd.h>

using namespace std;



AudioCapture::AudioCapture(const char* device_name, int sample_rate, int format,
    int buffer_size) : format(format) {
  this->device = alcCaptureOpenDevice(device_name, sample_rate, format,
      buffer_size);
  al_check_error();
  alcCaptureStart(this->device);
  al_check_error();
}

AudioCapture::~AudioCapture() {
  alcCaptureStop(this->device);
  al_check_error();
  alcCaptureCloseDevice(this->device);
  al_check_error();
}

size_t AudioCapture::get_samples(void* buffer, size_t sample_count, bool wait) {
  return this->get_frames(
      buffer,
      sample_count / (1 + is_stereo(this->format)),
      wait);
}

size_t AudioCapture::get_frames(void* buffer, size_t frame_count, bool wait) {
  size_t frames_read = 0;
  size_t frames_to_get;
  do {
    int frames_available_int = 0;
    alcGetIntegerv(this->device, ALC_CAPTURE_SAMPLES, sizeof(ALint),
        &frames_available_int);
    size_t frames_available = static_cast<size_t>(frames_available_int);

    frames_to_get = (frame_count > frames_available)
        ? frames_available : frame_count;
    al_check_error();
    alcCaptureSamples(this->device, buffer, frames_to_get);
    frames_read += frames_to_get;
    frame_count -= frames_to_get;
    al_check_error();

    if (wait && (frames_to_get < frame_count)) {
      size_t bytes_read = bytes_per_frame(this->format) * frames_to_get;
      buffer = reinterpret_cast<char*>(buffer) + bytes_read;

      usleep(1000); // Don't busy-wait; yield for at least 1ms (usually 10+ ms)
    }
  } while (wait && frame_count);
  return frames_read;
}
