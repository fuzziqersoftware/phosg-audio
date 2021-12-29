#pragma once

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

#include <vector>


struct WAVLoop {
  size_t start;
  size_t end;
  uint8_t type;
};

struct WAVContents {
  std::vector<float> samples;
  size_t num_channels;
  size_t sample_rate;
  int64_t base_note; // -1 if not specified
  std::vector<WAVLoop> loops;

  WAVContents();

  float seconds() const;
};

WAVContents load_wav(const char* filename);
WAVContents load_wav(FILE* f);

void save_wav(const char* filename, const std::vector<uint8_t>& samples,
    size_t sample_rate, size_t num_channels);
void save_wav(const char* filename, const std::vector<int16_t>& samples,
    size_t sample_rate, size_t num_channels);
void save_wav(const char* filename, const std::vector<float>& samples,
    size_t sample_rate, size_t num_channels);
