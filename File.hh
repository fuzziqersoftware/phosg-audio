#include <inttypes.h>
#include <stddef.h>

#include <vector>


struct WAVContents {
  std::vector<float> samples;
  size_t num_channels;
  size_t sample_rate;

  float seconds() const;
};

WAVContents load_wav(const char* filename);
WAVContents load_wav(FILE* f);

void save_wav(const char* filename, const std::vector<int16_t>& samples,
    size_t sample_rate, size_t num_channels);
void save_wav(const char* filename, const std::vector<float>& samples,
    size_t sample_rate, size_t num_channels);
