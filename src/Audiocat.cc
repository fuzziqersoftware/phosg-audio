#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <complex>
#include <memory>
#include <phosg/Encoding.hh>

#include "Constants.hh"
#include "Convert.hh"
#include "Capture.hh"
#include "FourierTransform.hh"
#include "Sound.hh"
#include "Stream.hh"

using namespace std;



void print_usage() {
  fprintf(stderr, "\
audiocat can do three things:\n\
  audiocat --listen [options]\n\
      Listen using the default input device and output sound data on stdout.\n\
  audiocat --play [options]\n\
      Read data on stdin and play it using the default output device.\n\
  audiocat --wave=WAVE [options]\n\
      Generate a sound and output it on stdout. If --play is also given, play\n\
      the generated sound using the default output device instead.\n\
\n\
Options:\n\
  --verbose\n\
      Show status messages while working.\n\
  --buffer-limit=LIMIT\n\
      Store this many samples in each OpenAL buffer (default 2048).\n\
  --buffer-count=COUNT\n\
      Allocate this many OpenAL buffers. audiocat will fill these buffers as\n\
      soon as they become available, so this effectively controls how far\n\
      audiocat reads fomr stdin ahead of the playback device.\n\
  --sample-rate=SAMPLE-RATE\n\
      Play or record sound at this sample rate (default 44100).\n\
  --format=FORMAT\n\
      Play or record sound in this format. Valid values are mono-i8, stereo-i8,\n\
      mono-i16 (default), stereo-i16, mono-f32, and stereo-f32.\n\
  --reverse-endian\n\
      For 16-bit and 32-bit formats, byteswap each sample before playing it\n\
      (with --play) or before writing it to stdout (with --listen).\n\
  --freq=FREQUENCY\n\
      When generating sounds, generate tones at this frequency.\n\
  --note=NOTE\n\
      When generating sounds, generate this note (example: F#5).\n\
  --duration=DURATION\n\
      Listen or generate sound for this many seconds. No effect when playing;\n\
      audiocat will always play all data from stdin.\n\
  --output-format=DISPLAY-FORMAT\n\
      When listening, output captured audio in this format. Valid formats are\n\
      binary (default), text, and fourier-histogram.\n\
  --fourier-width=WIDTH\n\
      With the fourier-histogram output format, set the number of samples to\n\
      transform on each line of output (must be a power of 2; default 4096).\n\
");
}



enum class OutputFormat {
  Binary = 0,
  Text,
  FFTHistogram,
};



int main(int argc, char* argv[]) {

  bool verbose = false;
  bool listen = false;
  bool play = false;
  int sample_rate = 44100;
  const char* wave_type = NULL;
  int frequency;
  double duration = 0.0; // indefinite
  size_t buffer_limit = 2048;
  size_t buffer_count = 4;
  size_t fourier_width = 4096;
  bool reverse_endian = false;
  const char* format_name = "mono-i16";
  OutputFormat output_format = OutputFormat::Binary;
  for (int x = 1; x < argc; x++) {
    if (!strcmp(argv[x], "--verbose")) {
      verbose = true;
    } else if (!strcmp(argv[x], "--listen")) {
      listen = true;
    } else if (!strcmp(argv[x], "--play")) {
      play = true;
    } else if (!strncmp(argv[x], "--buffer-limit=", 15)) {
      buffer_limit = strtoull(&argv[x][15], NULL, 0);
    } else if (!strncmp(argv[x], "--buffer-count=", 15)) {
      buffer_count = strtoull(&argv[x][15], NULL, 0);
    } else if (!strncmp(argv[x], "--sample-rate=", 14)) {
      sample_rate = atoi(&argv[x][14]);
    } else if (!strncmp(argv[x], "--format=", 9)) {
      format_name = &argv[x][9];
    } else if (!strcmp(argv[x], "--output-format=binary")) {
      output_format = OutputFormat::Binary;
    } else if (!strcmp(argv[x], "--output-format=text")) {
      output_format = OutputFormat::Text;
    } else if (!strcmp(argv[x], "--output-format=fourier-histogram")) {
      output_format = OutputFormat::FFTHistogram;
    } else if (!strncmp(argv[x], "--fourier-width=", 16)) {
      fourier_width = strtoull(&argv[x][16], NULL, 0);
    } else if (!strcmp(argv[x], "--reverse-endian")) {
      reverse_endian = true;
    } else if (!strncmp(argv[x], "--wave=", 7)) {
      wave_type = &argv[x][7];
    } else if (!strncmp(argv[x], "--freq=", 7)) {
      frequency = atoi(&argv[x][7]);
    } else if (!strncmp(argv[x], "--note=", 7)) {
      uint8_t note = note_for_name(&argv[x][7]);
      frequency = frequency_for_note(note);
    } else if (!strncmp(argv[x], "--duration=", 11)) {
      duration = atof(&argv[x][11]);
    } else {
      fprintf(stderr, "unrecognized option: %s\n", argv[x]);
      return 1;
    }
  }

  init_al();

  int format = format_for_name(format_name);
  size_t bpf = bytes_per_frame(format);

  if (listen) {
    size_t samples_captured = 0;
    {
      if (verbose) {
        if (duration) {
          fprintf(stderr,
              "listening for %g seconds of %s data at %dHz, writing to stdout\n",
              duration, name_for_format(format), sample_rate);
        } else {
          fprintf(stderr,
              "listening for %s data at %dHz, writing to stdout\n",
              name_for_format(format), sample_rate);
        }
      }
      AudioCapture cap(NULL, sample_rate, format, sample_rate);

      size_t sample_limit = duration * sample_rate;
      if (output_format == OutputFormat::FFTHistogram) {
        void* buffer = malloc(bpf * fourier_width);
        while (!sample_limit || (samples_captured < sample_limit)) {
          size_t sample_count = cap.get_samples(buffer, fourier_width, true);
          if (sample_count != fourier_width) {
            fprintf(stderr,
                "expected %zu samples, got %zu\n", fourier_width, sample_count);
            throw logic_error("blocking read did not produce enough data");
          }
          vector<complex<double>> samples_complex = make_complex_multi(
              reinterpret_cast<const float*>(buffer), fourier_width);
          auto fourier_ret = compute_fourier_transform(samples_complex);

          size_t compress_factor = 21;
          size_t cell_count = (fourier_width / compress_factor) +
              ((fourier_width % compress_factor) ? 1 : 0);

          float max_intensity = 0.0;
          vector<float> cell_intensity(cell_count, 0.0);
          for (size_t x = 0; x < fourier_ret.size(); x++) {
            float& cell = cell_intensity[x / compress_factor];
            cell += sqrt(norm(fourier_ret[x]));
            if (cell > max_intensity) {
              max_intensity = cell;
            }
          }

          string line_data(cell_count, ' ');
          const string intensity_chars(" .:+*#@");
          for (size_t x = 0; x < cell_intensity.size(); x++) {
            // intentional float truncation
            size_t intensity_class = cell_intensity[x] * intensity_chars.size()
                / max_intensity;
            if (intensity_class >= intensity_chars.size()) {
              intensity_class = intensity_chars.size() - 1;
            }
            line_data[x] = intensity_chars[intensity_class];
          }

          fprintf(stdout, "%s\n", line_data.c_str());
          fflush(stdout);

          samples_captured += fourier_width;
        }
        free(buffer);

      } else {
        void* buffer = malloc(bpf * sample_rate);
        while (!sample_limit || (samples_captured < sample_limit)) {
          usleep(10000);
          size_t samples_this_period = sample_limit
              ? (sample_limit - samples_captured)
              : sample_rate;
          size_t sample_count = cap.get_samples(buffer, samples_this_period);
          if (reverse_endian) {
            byteswap_samples(buffer, sample_count, format);
          }
          if (output_format == OutputFormat::Binary) {
            fwrite(buffer, 1, bpf * sample_count, stdout);
            fflush(stdout);
          } else {
            throw logic_error("text output not implemented");
          }
          samples_captured += sample_count;
        }
        free(buffer);
      }
    }

    if (verbose) {
      fprintf(stderr, "done listening (%g seconds)\n", duration);
    }

  } else if (wave_type) {
    shared_ptr<GeneratedSound> sound;
    if (!strcmp(wave_type, "sine")) {
      sound.reset(new SineWave(frequency, duration, 1.0, sample_rate));
    } else if (!strcmp(wave_type, "square")) {
      sound.reset(new SquareWave(frequency, duration, 1.0, sample_rate));
    } else if (!strcmp(wave_type, "triangle")) {
      sound.reset(new TriangleWave(frequency, duration, 1.0, sample_rate));
    } else if (!strcmp(wave_type, "front-triangle")) {
      sound.reset(new FrontTriangleWave(frequency, duration, 1.0, sample_rate));
    } else if (!strcmp(wave_type, "white-noise")) {
      sound.reset(new WhiteNoise(duration, 1.0, sample_rate));
    } else if (!strcmp(wave_type, "split-noise")) {
      sound.reset(new SplitNoise(frequency, duration, 1.0, sample_rate));
    } else {
      fprintf(stderr, "unsupported wave type: %s\n", wave_type);
      return 1;
    }

    if (play) {
      if (verbose) {
        fprintf(stderr,
            "playing generated %d Hz %s waveform at %dHz for %g seconds\n",
            frequency, wave_type, sample_rate, duration);
      }
      sound->play();
      usleep(duration * 1000000);

    } else {
      if (verbose) {
        fprintf(stderr,
            "generating %d Hz %s waveform at %dHz (%g seconds)\n",
            frequency, wave_type, sample_rate, duration);
      }
      sound->write(stdout);
    }

  } else if (play) {
    if (verbose) {
      fprintf(stderr,
          "playing %s data at %dHz from stdin\n",
          name_for_format(format), sample_rate);
    }

    // High watermark: 1 second, low watermark: 1/8 second. This means we try to
    // read up to 1 second of data at a time, but we'll start playing if we read
    // 1/8 second or more.
    size_t buffer_size = bpf * buffer_limit;
    size_t low_watermark = buffer_limit / 8;
    void* buffer = malloc(buffer_size);

    // Open a stream and forward samples from stdin to it
    AudioStream stream(sample_rate, format, buffer_count);
    size_t buffer_samples = 0;
    while (!feof(stdin)) {
      ssize_t samples_read = fread(
          ((uint8_t*)buffer) + (buffer_samples * bpf),
          bpf, buffer_limit - buffer_samples, stdin);
      if (samples_read <= 0) {
        continue;
      }
      buffer_samples += samples_read;

      if (buffer_samples >= low_watermark) {
        if (reverse_endian) {
          byteswap_samples(buffer, buffer_samples, format);
        }
        stream.add_samples(buffer, buffer_samples);
        buffer_samples = 0;
      }
    }

    // Reached EOF; write any remaining samples
    if (buffer_samples) {
      if (reverse_endian) {
        byteswap_samples(buffer, buffer_samples, format);
      }
      stream.add_samples(buffer, buffer_samples);
    }

    // Wait for the sound to finish playing
    stream.wait();

    if (verbose) {
      fprintf(stderr, "(done) playing %s data at %d kHz from stdin\n",
          name_for_format(format), sample_rate);
    }

  } else {
    fprintf(stderr, "one of --play, --listen, or --wave must be given\n");
    print_usage();
    exit_al();
    return 2;
  }

  return 0;
}
