#include "File.hh"

#include <inttypes.h>

#include <phosg/Encoding.hh>
#include <phosg/Filesystem.hh>
#include <phosg/Strings.hh>
#include <vector>

using namespace std;



WAVContents::WAVContents() : num_channels(0), sample_rate(0), base_note(-1) { }

float WAVContents::seconds() const {
  return static_cast<float>(this->samples.size()) / this->sample_rate;
}



struct SaveWAVHeader {
  uint32_t riff_magic;   // 0x52494646
  uint32_t file_size;    // size of file - 8
  uint32_t wave_magic;   // 0x57415645

  uint32_t fmt_magic;    // 0x666d7420
  uint32_t fmt_size;     // 16
  uint16_t format;       // 1 = PCM
  uint16_t num_channels;
  uint32_t sample_rate;
  uint32_t byte_rate;    // num_channels * sample_rate * bits_per_sample / 8
  uint16_t block_align;  // num_channels * bits_per_sample / 8
  uint16_t bits_per_sample;

  uint32_t data_magic;   // 0x64617461
  uint32_t data_size;    // num_samples * num_channels * bits_per_sample / 8

  SaveWAVHeader(uint32_t num_samples, uint16_t num_channels,
      uint32_t sample_rate, uint16_t bits_per_sample, bool is_float) {
    this->riff_magic = bswap32(0x52494646);
    this->file_size = num_samples * num_channels * bits_per_sample / 8 +
        sizeof(SaveWAVHeader) - 8;
    this->wave_magic = bswap32(0x57415645);
    this->fmt_magic = bswap32(0x666d7420);
    this->fmt_size = 16;
    this->format = is_float ? 3 : 1;
    this->num_channels = num_channels;
    this->sample_rate = sample_rate;
    this->byte_rate = num_channels * sample_rate * bits_per_sample / 8;
    this->block_align = num_channels * bits_per_sample / 8;
    this->bits_per_sample = bits_per_sample;
    this->data_magic = bswap32(0x64617461);
    this->data_size = num_samples * num_channels * bits_per_sample / 8;
  }
};

void save_wav(const char* filename, const vector<uint8_t>& samples,
    size_t sample_rate, size_t num_channels) {
  SaveWAVHeader header(samples.size(), num_channels, sample_rate, 8, false);
  auto f = fopen_unique(filename, "wb");
  fwrite(&header, sizeof(SaveWAVHeader), 1, f.get());
  fwrite(samples.data(), sizeof(uint8_t), samples.size(), f.get());
}

void save_wav(const char* filename, const vector<int16_t>& samples,
    size_t sample_rate, size_t num_channels) {
  SaveWAVHeader header(samples.size(), num_channels, sample_rate, 16, false);
  auto f = fopen_unique(filename, "wb");
  fwrite(&header, sizeof(SaveWAVHeader), 1, f.get());
  fwrite(samples.data(), sizeof(int16_t), samples.size(), f.get());
}

void save_wav(const char* filename, const vector<float>& samples,
    size_t sample_rate, size_t num_channels) {
  SaveWAVHeader header(samples.size(), num_channels, sample_rate, 32, true);
  auto f = fopen_unique(filename, "wb");
  fwrite(&header, sizeof(SaveWAVHeader), 1, f.get());
  fwrite(samples.data(), sizeof(float), samples.size(), f.get());
}



// note: this isn't the same as SaveWAVHeader; that structure is only used to
// write wav files. when loading files, we might encounter chunks that this
// program never creates by default, but we should be able to handle them
// anyway, so the structure is split here

struct RIFFHeader {
  uint32_t riff_magic;   // 0x52494646 big-endian ('RIFF')
  uint32_t file_size;    // size of file - 8
};

struct WAVEHeader {
  uint32_t wave_magic;   // 0x57415645 big-endian ('WAVE')
  uint32_t fmt_magic;    // 0x666d7420
  uint32_t fmt_size;     // 16
  uint16_t format;       // 1 = PCM, 3 = float
  uint16_t num_channels;
  uint32_t sample_rate;
  uint32_t byte_rate;    // num_channels * sample_rate * bits_per_sample / 8
  uint16_t block_align;  // num_channels * bits_per_sample / 8
  uint16_t bits_per_sample;
};

struct RIFFChunkHeader {
  uint32_t magic;
  uint32_t size;
};

struct SampleChunkHeader {
  uint32_t manufacturer;
  uint32_t product;
  uint32_t sample_period;
  uint32_t base_note;
  uint32_t pitch_fraction;
  uint32_t smtpe_format;
  uint32_t smtpe_offset;
  uint32_t num_loops;
  uint32_t sampler_data;

  struct {
    uint32_t loop_cue_point_id; // can be zero? we'll only have at most one loop in this context
    uint32_t loop_type; // 0 = normal, 1 = ping-pong, 2 = reverse
    uint32_t loop_start; // start and end are byte offsets into the wave data, not sample indexes
    uint32_t loop_end;
    uint32_t loop_fraction; // fraction of a sample to loop (0)
    uint32_t loop_play_count; // 0 = loop forever
  } loops[0];
};


WAVContents load_wav(const char* filename) {
  auto f = fopen_unique(filename, "rb");
  return load_wav(f.get());
}

WAVContents load_wav(FILE* f) {
  // check the RIFF header
  uint32_t magic;
  freadx(f, &magic, sizeof(uint32_t));
  if (magic != 0x46464952) {
    throw runtime_error(string_printf("unknown file format: %08" PRIX32, magic));
  }

  uint32_t file_size;
  freadx(f, &file_size, sizeof(uint32_t));

  WAVContents contents;

  // iterate over the chunks
  WAVEHeader wav;
  wav.wave_magic = 0;
  for (;;) {
    RIFFChunkHeader chunk_header;
    freadx(f, &chunk_header, sizeof(RIFFChunkHeader));

    if (chunk_header.magic == 0x45564157) { // 'WAVE'
      memcpy(&wav, &chunk_header, sizeof(RIFFChunkHeader));
      freadx(f, reinterpret_cast<uint8_t*>(&wav) + sizeof(RIFFChunkHeader),
          sizeof(WAVEHeader) - sizeof(RIFFChunkHeader));

      // check the header info. we only support 1-channel, 16-bit sounds for now
      if (wav.wave_magic != 0x45564157) { // 'WAVE'
        throw runtime_error(string_printf("sound has incorrect wave_magic (%" PRIX32 ")", wav.wave_magic));
      }
      if (wav.fmt_magic != 0x20746D66) { // 'fmt '
        throw runtime_error(string_printf("sound has incorrect fmt_magic (%" PRIX32 ")", wav.fmt_magic));
      }
      if (wav.num_channels > 2) {
        throw runtime_error(string_printf("sound has too many channels (%" PRIu16 ")", wav.num_channels));
      }

      contents.sample_rate = wav.sample_rate;
      contents.num_channels = wav.num_channels;

    } else if (chunk_header.magic == 0x6C706D73) { // 'smpl'
      if (wav.wave_magic == 0) {
        throw runtime_error("smpl chunk is before WAVE chunk");
      }

      const string data = freadx(f, chunk_header.size);
      const SampleChunkHeader* sample_header = reinterpret_cast<const SampleChunkHeader*>(data.data());
      const char* last_loop_ptr = data.data() + data.size() - sizeof(sample_header->loops[0]);

      contents.base_note = sample_header->base_note;
      contents.loops.resize(sample_header->num_loops);
      for (size_t x = 0; x < sample_header->num_loops; x++) {
        auto& contents_loop = contents.loops[x];
        auto* header_loop = &sample_header->loops[x];
        if (reinterpret_cast<const char*>(header_loop) > last_loop_ptr) {
          throw runtime_error("sound has malformed loop information");
        }
        contents_loop.start = header_loop->loop_start / (wav.bits_per_sample >> 3);
        contents_loop.end = header_loop->loop_end / (wav.bits_per_sample >> 3);
        contents_loop.type = header_loop->loop_type;
      }

    } else if (chunk_header.magic == 0x61746164) { // 'data'
      if (wav.wave_magic == 0) {
        throw runtime_error("data chunk is before WAVE chunk");
      }

      contents.samples.resize((8 * chunk_header.size) / wav.bits_per_sample);

      // 32-bit float
      if ((wav.format == 3) && (wav.bits_per_sample == 32)) {
        freadx(f, contents.samples.data(), contents.samples.size() * sizeof(float));

      // 16-bit signed int
      } else if ((wav.format == 1) && (wav.bits_per_sample == 16)) {
        vector<int16_t> int_samples(contents.samples.size());
        freadx(f, int_samples.data(), int_samples.size() * sizeof(int16_t));
        for (size_t x = 0; x < int_samples.size(); x++) {
          if (int_samples[x] == -0x8000) {
            contents.samples[x] = -1.0f;
          } else {
            contents.samples[x] = static_cast<float>(int_samples[x]) / 32767.0f;
          }
        }

      // 8-bit unsigned int
      } else if ((wav.format == 1) && (wav.bits_per_sample == 8)) {
        vector<uint8_t> int_samples(contents.samples.size());
        freadx(f, int_samples.data(), int_samples.size() * sizeof(uint8_t));
        for (size_t x = 0; x < int_samples.size(); x++) {
          contents.samples[x] = (static_cast<float>(int_samples[x]) / 128.0f) - 1.0f;
        }
      } else {
        throw runtime_error("sample width is not supported (format=%" PRIu16 ", bits_per_sample=%" PRIu16 ")");
      }

      break;

    } else {
      fseek(f, chunk_header.size, SEEK_CUR);
    }
  }

  // if there are loops, convert the byte offsets to 

  return contents;
}
