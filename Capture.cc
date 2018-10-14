#include "Capture.hh"

using namespace std;



AudioCapture::AudioCapture(const char* device_name, int sample_rate, int format,
    int buffer_size) {
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

size_t AudioCapture::get_samples(void* buffer, size_t sample_count) {
  int samples_available;
  alcGetIntegerv(this->device, ALC_CAPTURE_SAMPLES, sizeof(ALint),
      &samples_available);
  if (sample_count > samples_available) {
    sample_count = samples_available;
  }
  al_check_error();
  alcCaptureSamples(this->device, buffer, sample_count);
  al_check_error();
  return sample_count;
}
