#pragma once

#include <stdint.h>

#include <vector>

void byteswap_samples16(void* buffer, size_t sample_count, bool stereo);
void byteswap_samples32(void* buffer, size_t sample_count, bool stereo);
void byteswap_samples(void* buffer, size_t sample_count, int format);

std::vector<float> convert_samples_s16_to_f32(const std::vector<int16_t>& samples);
std::vector<float> convert_samples_u16_to_f32(const std::vector<uint16_t>& samples);
std::vector<float> convert_samples_s8_to_f32(const std::vector<int8_t>& samples);
std::vector<float> convert_samples_u8_to_f32(const std::vector<uint8_t>& samples);
std::vector<int16_t> convert_samples_f32_to_s16(const std::vector<float>& samples);
std::vector<uint16_t> convert_samples_f32_to_u16(const std::vector<float>& samples);
std::vector<int8_t> convert_samples_f32_to_s8(const std::vector<float>& samples);
std::vector<uint8_t> convert_samples_f32_to_u8(const std::vector<float>& samples);
