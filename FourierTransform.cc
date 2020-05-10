#include "FourierTransform.hh"

using namespace std;



static const double pi = 3.14159265358979323846;



vector<complex<double>> make_complex_multi(const vector<float>& input) {
  return make_complex_multi(input.data(), input.size());
}

vector<complex<double>> make_complex_multi(const float* input, size_t count) {
  vector<complex<double>> res;
  res.reserve(count);
  for (size_t x = 0; x < count; x++) {
    res.emplace_back(input[x], 0.0);
  }
  return res;
}



void compute_fourier_transform_recursive(const vector<complex<double>>& input,
    size_t input_start_index, size_t input_stride, size_t input_count,
    vector<complex<double>>& output, size_t output_start_index) {
  if (input_count == 1) {
    output[output_start_index] = input[input_start_index];
    return;
  }

  size_t half_count = input_count / 2;

  compute_fourier_transform_recursive(input,
      input_start_index, input_stride * 2, half_count, output,
      output_start_index);
  compute_fourier_transform_recursive(input,
      input_start_index + input_stride, input_stride * 2, half_count, output,
      output_start_index + half_count);

  // TODO: precompute the roots of unity to make this faster
  for (size_t z = 0; z < input_count / 2; z++) {
    size_t index = z + output_start_index;
    complex<double> v = polar(1.0, (-2.0 * pi * z) / input_count) * output[index + half_count];
    complex<double> t = output[index];
    output[index] = t + v;
    output[index + half_count] = t - v;
  }
}

vector<complex<double>> compute_fourier_transform(
    const vector<complex<double>>& input) {
  vector<complex<double>> output(input.size());
  compute_fourier_transform_recursive(input, 0, 1, input.size(), output, 0);
  return output;
}
