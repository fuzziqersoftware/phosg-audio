#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <memory>
#include <phosg/Encoding.hh>

#include "Constants.hh"
#include "Capture.hh"
#include "Sound.hh"
#include "Stream.hh"

using namespace std;


struct stereo8_sample {
  int8_t left;
  int8_t right;
};

struct stereo16_sample {
  int16_t left;
  int16_t right;
};



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
");
}



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
  bool reverse_endian = false;
  const char* format_name = "mono-i16";
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
    }
  }

  init_al();

  int format = format_for_name(format_name);

  if (listen) {
    size_t sample_limit = duration * sample_rate;
    void* buffer = malloc(bytes_per_sample(format) * sample_rate);
    size_t samples_captured = 0;
    {
      if (verbose) {
        if (duration) {
          fprintf(stderr, "(start) listening for %g seconds of %s data at %d kHz, writing to stdout\n",
              duration, name_for_format(format), sample_rate);
        } else {
          fprintf(stderr, "(start) listening for %s data at %d kHz, writing to stdout\n",
              name_for_format(format), sample_rate);
        }
      }
      AudioCapture cap(NULL, sample_rate, format, sample_rate);

      size_t bps = bytes_per_sample(format);
      while (!sample_limit || (samples_captured < sample_limit)) {
        usleep(10000);
        size_t samples_this_period = sample_limit ? (sample_limit - samples_captured) : sample_rate;
        size_t sample_count = cap.get_samples(buffer, samples_this_period);
        if (reverse_endian) {
          byteswap_samples(buffer, sample_count, format);
        }
        fwrite(buffer, 1, bps * sample_count, stdout);
        fflush(stdout);
        samples_captured += sample_count;
      }
    }
    free(buffer);

    if (verbose) {
      fprintf(stderr, "(done) listening for %g seconds of %s data at %d kHz, writing to stdout\n",
          duration, name_for_format(format), sample_rate);
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
        fprintf(stderr, "playing generated %d Hz %s waveform at %d kHz for %g seconds\n",
            frequency, wave_type, sample_rate, duration);
      }
      sound->play();
      usleep(duration * 1000000);

    } else {
      if (verbose) {
        fprintf(stderr, "generating %d Hz %s waveform at %d kHz (%g seconds)\n",
            frequency, wave_type, sample_rate, duration);
      }
      sound->write(stdout);
    }

  } else if (play) {
    if (verbose) {
      fprintf(stderr, "(start) playing %s data at %d kHz from stdin\n",
          name_for_format(format), sample_rate);
    }

    // high watermark: 1 second, low watermark: 1/8 second. this means we try to
    // read up to 1 second of data at a time, but we'll start playing if we read
    // 1/8 second or more
    size_t bps = bytes_per_sample(format);
    size_t buffer_size = bps * buffer_limit;
    size_t low_watermark = buffer_limit / 8;
    void* buffer = malloc(buffer_size);

    // open a stream and forward samples from stdin to it
    AudioStream stream(sample_rate, format, buffer_count);
    size_t buffer_samples = 0;
    while (!feof(stdin)) {
      ssize_t samples_read = fread(
          ((uint8_t*)buffer) + (buffer_samples * bps),
          bps, buffer_limit - buffer_samples, stdin);
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

    // reached EOF; write any remaining samples
    if (buffer_samples) {
      if (reverse_endian) {
        byteswap_samples(buffer, buffer_samples, format);
      }
      stream.add_samples(buffer, buffer_samples);
    }

    // wait for the sound to finish playing
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
