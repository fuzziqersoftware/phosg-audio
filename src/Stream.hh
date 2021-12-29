#pragma once

#include "Constants.hh"

#include <string>
#include <unordered_map>
#include <unordered_set>



class AudioStream {
public:
  AudioStream(int sample_rate, int format, size_t num_buffers = 16);
  ~AudioStream();

  void add_samples(const void* buffer, size_t sample_count);
  void add_frames(const void* buffer, size_t frame_count);

  void wait();

  size_t check_buffers();
  size_t buffer_count() const;
  size_t available_buffer_count() const;
  size_t queued_buffer_count() const;

private:
  void wait_for_buffers(size_t num_buffers = 1);

  int sample_rate;
  int format;

  std::vector<ALuint> all_buffer_ids;
  std::unordered_set<ALuint> available_buffer_ids;
  std::unordered_set<ALuint> queued_buffer_ids;
  std::unordered_map<ALuint, std::string> buffer_id_to_data;
  ALuint source_id;
};
