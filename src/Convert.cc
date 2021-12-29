#include "Constants.hh"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <phosg/Encoding.hh>

#include <stdexcept>

using namespace std;


void byteswap_samples16(void* buffer, size_t sample_count, bool stereo) {
  if (stereo) {
    sample_count *= 2;
  }

  int16_t* samples = reinterpret_cast<int16_t*>(buffer);
  for (size_t x = 0; x < sample_count; x++) {
    samples[x] = bswap16(samples[x]);
  }
}

void byteswap_samples32(void* buffer, size_t sample_count, bool stereo) {
  if (stereo) {
    sample_count *= 2;
  }

  int32_t* samples = reinterpret_cast<int32_t*>(buffer);
  for (size_t x = 0; x < sample_count; x++) {
    samples[x] = bswap32(samples[x]);
  }
}

void byteswap_samples(void* buffer, size_t sample_count, int format) {
  if (is_16bit(format)) {
    byteswap_samples16(buffer, sample_count, is_stereo(format));
  } else if (is_32bit(format)) {
    byteswap_samples32(buffer, sample_count, is_stereo(format));
  }
}

// Conversion from f32

static inline int16_t convert_sample_f32_to_s16(float sample) {
  if (sample >= 1.0f) {
    return 0x7FFF;
  } else if (sample <= -1.0f) {
    return -0x8000;
  } else {
    return static_cast<int16_t>(sample * 32767.0f);
  }
}

static inline uint16_t convert_sample_f32_to_u16(float sample) {
  if (sample >= 1.0f) {
    return 0xFFFF;
  } else if (sample <= -1.0f) {
    return 0;
  } else {
    return static_cast<int16_t>((sample + 1.0) * 32767.0f);
  }
}

static inline int8_t convert_sample_f32_to_s8(float sample) {
  if (sample >= 1.0f) {
    return 0x7F;
  } else if (sample <= -1.0f) {
    return -0x80;
  } else {
    return static_cast<int16_t>(sample * 127.0f);
  }
}

static inline uint8_t convert_sample_f32_to_u8(float sample) {
  if (sample >= 1.0f) {
    return 0xFF;
  } else if (sample <= -1.0f) {
    return 0;
  } else {
    return static_cast<int16_t>((sample + 1.0) * 127.0f);
  }
}

// Conversion to f32

static inline float convert_sample_s16_to_f32(int16_t sample) {
  if (sample == -0x8000) {
    return -1.0f;
  } else {
    return static_cast<float>(sample) / 32767.0f;
  }
}

static inline float convert_sample_u16_to_f32(uint16_t sample) {
  return static_cast<float>(sample) / 32767.0f - 1.0;
}

static inline float convert_sample_s8_to_f32(int8_t sample) {
  if (sample == -0x80) {
    return -1.0f;
  } else {
    return static_cast<float>(sample) / 127.0f;
  }
}

static inline float convert_sample_u8_to_f32(uint8_t sample) {
  return static_cast<float>(sample) / 127.0f - 1.0;
}



template <typename InSampleT, typename OutSampleT, OutSampleT (*ConvertFn)(InSampleT)>
vector<OutSampleT> convert_samples(const vector<InSampleT>& samples) {
  vector<OutSampleT> ret;
  ret.reserve(samples.size());
  for (InSampleT sample : samples) {
    ret.emplace_back(ConvertFn(sample));
  }
  return ret;
}

vector<float> convert_samples_s16_to_f32(const vector<int16_t>& samples) {
  return convert_samples<int16_t, float, convert_sample_s16_to_f32>(samples);
}

vector<float> convert_samples_u16_to_f32(const vector<uint16_t>& samples) {
  return convert_samples<uint16_t, float, convert_sample_u16_to_f32>(samples);
}

vector<float> convert_samples_s8_to_f32(const vector<int8_t>& samples) {
  return convert_samples<int8_t, float, convert_sample_s8_to_f32>(samples);
}

vector<float> convert_samples_u8_to_f32(const vector<uint8_t>& samples) {
  return convert_samples<uint8_t, float, convert_sample_u8_to_f32>(samples);
}

vector<int16_t> convert_samples_f32_to_s16(const vector<float>& samples) {
  return convert_samples<float, int16_t, convert_sample_f32_to_s16>(samples);
}

vector<uint16_t> convert_samples_f32_to_u16(const vector<float>& samples) {
  return convert_samples<float, uint16_t, convert_sample_f32_to_u16>(samples);
}

vector<int8_t> convert_samples_f32_to_s8(const vector<float>& samples) {
  return convert_samples<float, int8_t, convert_sample_f32_to_s8>(samples);
}

vector<uint8_t> convert_samples_f32_to_u8(const vector<float>& samples) {
  return convert_samples<float, uint8_t, convert_sample_f32_to_u8>(samples);
}
