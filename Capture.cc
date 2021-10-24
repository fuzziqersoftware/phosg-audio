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
  size_t samples_read = 0;
  size_t samples_to_get;
  do {
    int samples_available_int = 0;
    alcGetIntegerv(this->device, ALC_CAPTURE_SAMPLES, sizeof(ALint),
        &samples_available_int);
    size_t samples_available = static_cast<size_t>(samples_available_int);

    samples_to_get = (sample_count > samples_available) ? samples_available : sample_count;
    al_check_error();
    alcCaptureSamples(this->device, buffer, samples_to_get);
    samples_read += samples_to_get;
    sample_count -= samples_to_get;
    al_check_error();

    if (wait && (samples_to_get < sample_count)) {
      size_t bytes_read = bytes_per_sample(this->format) * samples_to_get;
      buffer = reinterpret_cast<char*>(buffer) + bytes_read;

      usleep(1000); // Don't busy-wait; yield for at least 1ms (usually 10+ ms)
    }
  } while (wait && sample_count);
  return samples_read;
}
