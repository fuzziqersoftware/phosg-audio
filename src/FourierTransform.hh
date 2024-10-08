#pragma once

#include <complex>
#include <vector>

namespace phosg_audio {

std::vector<std::complex<double>> make_complex_multi(const std::vector<float>& input);
std::vector<std::complex<double>> make_complex_multi(const float* input, size_t count);

std::vector<std::complex<double>> compute_fourier_transform(const std::vector<std::complex<double>>& input);

} // namespace phosg_audio
