#include "Stream.hh"

#include <unistd.h>

using namespace std;



AudioStream::AudioStream(int sample_rate, int format, size_t num_buffers) :
    sample_rate(sample_rate), format(format), all_buffer_ids(num_buffers) {
  alGenBuffers(this->all_buffer_ids.size(), this->all_buffer_ids.data());
  al_check_error();

  alGenSources(1, &this->source_id);
  al_check_error();

  for (ALuint buffer_id : this->all_buffer_ids) {
    this->available_buffer_ids.emplace(buffer_id);
  }
}

AudioStream::~AudioStream() {
  alDeleteSources(1, &this->source_id);
  alDeleteBuffers(this->all_buffer_ids.size(), this->all_buffer_ids.data());
}

void AudioStream::add_samples(const void* buffer, size_t sample_count) {
  this->add_frames(buffer, sample_count / (1 + is_stereo(this->format)));
}

void AudioStream::add_frames(const void* buffer, size_t frame_count) {
  if (this->available_buffer_ids.empty()) {
    this->wait_for_buffers(1);
  } else {
    this->check_buffers();
  }

  ALuint buffer_id = *this->available_buffer_ids.begin();

  string& buffer_data = this->buffer_id_to_data[buffer_id];
  buffer_data.assign((const char*)buffer, frame_count * bytes_per_frame(this->format));

  // Add the new data to the buffer and queue it
  alBufferData(buffer_id, this->format, buffer_data.data(), buffer_data.size(),
      this->sample_rate);
  al_check_error();
  alSourceQueueBuffers(this->source_id, 1, &buffer_id);
  al_check_error();
  this->available_buffer_ids.erase(buffer_id);
  this->queued_buffer_ids.emplace(buffer_id);

  // Start playing the source if it isn't already playing
  ALint source_state;
  alGetSourcei(this->source_id, AL_SOURCE_STATE, &source_state);
  al_check_error();
  if (source_state != AL_PLAYING) {
    alSourcePlay(this->source_id);
    al_check_error();
  }
}

void AudioStream::wait() {
  // When all queued buffers are available, the sound is done playing
  this->wait_for_buffers(this->all_buffer_ids.size());
}

size_t AudioStream::check_buffers() {
  int buffers_processed;
  alGetSourcei(this->source_id, AL_BUFFERS_PROCESSED, &buffers_processed);
  al_check_error();
  if (buffers_processed) {
    vector<ALuint> buffer_ids(buffers_processed);
    alSourceUnqueueBuffers(this->source_id, buffers_processed, buffer_ids.data());
    al_check_error();
    for (ALuint buffer_id : buffer_ids) {
      this->available_buffer_ids.emplace(buffer_id);
      this->queued_buffer_ids.erase(buffer_id);
    }
  }
  return buffers_processed;
}

size_t AudioStream::buffer_count() const {
  return this->all_buffer_ids.size();
}

size_t AudioStream::available_buffer_count() const {
  return this->available_buffer_ids.size();
}

size_t AudioStream::queued_buffer_count() const {
  return this->queued_buffer_ids.size();
}

void AudioStream::wait_for_buffers(size_t num_buffers) {
  for (;;) {
    this->check_buffers();
    if (this->available_buffer_ids.size() >= num_buffers) {
      return;
    }
    usleep(1000);
  }
}
