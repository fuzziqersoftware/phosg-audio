#define __STDC_FORMAT_MACROS

#include "File.hh"

#include <inttypes.h>
#include <string.h>

#include <phosg/Encoding.hh>
#include <phosg/Filesystem.hh>
#include <phosg/Strings.hh>
#include <vector>

using namespace std;

namespace phosg_audio {

WAVContents::WAVContents() : num_channels(0), sample_rate(0), base_note(-1) {}

float WAVContents::seconds() const {
  return static_cast<float>(this->samples.size()) / this->sample_rate;
}

struct SaveWAVHeader {
  uint32_t riff_magic; // 0x52494646 ('RIFF')
  uint32_t file_size; // size of file - 8
  uint32_t wave_magic; // 0x57415645 ('WAVE')

  uint32_t fmt_magic; // 0x666d7420 ('fmt ')
  uint32_t fmt_size; // 16
  uint16_t format; // 1 = PCM
  uint16_t num_channels;
  uint32_t sample_rate;
  uint32_t byte_rate; // num_channels * sample_rate * bits_per_sample / 8
  uint16_t block_align; // num_channels * bits_per_sample / 8
  uint16_t bits_per_sample;

  uint32_t data_magic; // 0x64617461 (data)
  uint32_t data_size; // num_samples * num_channels * bits_per_sample / 8

  SaveWAVHeader(uint32_t num_samples, uint16_t num_channels,
      uint32_t sample_rate, uint16_t bits_per_sample, bool is_float) {
    this->riff_magic = phosg::bswap32(0x52494646);
    this->file_size = num_samples * num_channels * bits_per_sample / 8 +
        sizeof(SaveWAVHeader) - 8;
    this->wave_magic = phosg::bswap32(0x57415645);
    this->fmt_magic = phosg::bswap32(0x666d7420);
    this->fmt_size = 16;
    this->format = is_float ? 3 : 1;
    this->num_channels = num_channels;
    this->sample_rate = sample_rate;
    this->byte_rate = num_channels * sample_rate * bits_per_sample / 8;
    this->block_align = num_channels * bits_per_sample / 8;
    this->bits_per_sample = bits_per_sample;
    this->data_magic = phosg::bswap32(0x64617461);
    this->data_size = num_samples * num_channels * bits_per_sample / 8;
  }
};

void save_wav(const char* filename, const vector<uint8_t>& samples, size_t sample_rate, size_t num_channels) {
  SaveWAVHeader header(samples.size(), num_channels, sample_rate, 8, false);
  auto f = phosg::fopen_unique(filename, "wb");
  phosg::fwritex(f.get(), &header, sizeof(SaveWAVHeader));
  phosg::fwritex(f.get(), samples.data(), sizeof(uint8_t) * samples.size());
}

void save_wav(const char* filename, const vector<int16_t>& samples, size_t sample_rate, size_t num_channels) {
  SaveWAVHeader header(samples.size(), num_channels, sample_rate, 16, false);
  auto f = phosg::fopen_unique(filename, "wb");
  phosg::fwritex(f.get(), &header, sizeof(SaveWAVHeader));
  phosg::fwritex(f.get(), samples.data(), sizeof(int16_t) * samples.size());
}

void save_wav(const char* filename, const vector<float>& samples, size_t sample_rate, size_t num_channels) {
  SaveWAVHeader header(samples.size(), num_channels, sample_rate, 32, true);
  auto f = phosg::fopen_unique(filename, "wb");
  phosg::fwritex(f.get(), &header, sizeof(SaveWAVHeader));
  phosg::fwritex(f.get(), samples.data(), sizeof(float) * samples.size());
}

// Note: this isn't the same as SaveWAVHeader; that structure is only used to
// write files. When loading files, we might encounter chunks that this library
// never creates by default, but we should be able to handle them anyway, so the
// structure is split here.

struct RIFFHeader {
  phosg::le_uint32_t riff_magic; // 0x52494646 ('RIFF')
  phosg::le_uint32_t file_size; // size of file - 8
} __attribute__((packed));

struct WAVEHeader {
  phosg::le_uint32_t wave_magic; // 0x57415645 ('WAVE')
  phosg::le_uint32_t fmt_magic; // 0x666d7420 ('fmt ')
  phosg::le_uint32_t fmt_size; // 16
  phosg::le_uint16_t format; // 1 = PCM, 3 = float
  phosg::le_uint16_t num_channels;
  phosg::le_uint32_t sample_rate;
  phosg::le_uint32_t byte_rate; // num_channels * sample_rate * bits_per_sample / 8
  phosg::le_uint16_t block_align; // num_channels * bits_per_sample / 8
  phosg::le_uint16_t bits_per_sample;
} __attribute__((packed));

struct RIFFChunkHeader {
  phosg::le_uint32_t magic;
  phosg::le_uint32_t size;
} __attribute__((packed));

struct SampleChunkHeader {
  phosg::le_uint32_t manufacturer;
  phosg::le_uint32_t product;
  phosg::le_uint32_t sample_period;
  phosg::le_uint32_t base_note;
  phosg::le_uint32_t pitch_fraction;
  phosg::le_uint32_t smpte_format;
  phosg::le_uint32_t smpte_offset;
  phosg::le_uint32_t num_loops;
  phosg::le_uint32_t sampler_data;

  struct {
    phosg::le_uint32_t cue_point_id;
    phosg::le_uint32_t type; // 0 = normal, 1 = ping-pong, 2 = reverse
    phosg::le_uint32_t start; // byte offset into the wave data
    phosg::le_uint32_t end; // byte offset into the wave data
    phosg::le_uint32_t fraction; // fraction of a sample to loop
    phosg::le_uint32_t play_count; // 0 = loop forever
  } loops[0];
} __attribute__((packed));

WAVContents load_wav(const char* filename) {
  auto f = phosg::fopen_unique(filename, "rb");
  return load_wav(f.get());
}

WAVContents load_wav(FILE* f) {
  {
    phosg::be_uint32_t magic;
    phosg::freadx(f, &magic, sizeof(uint32_t));
    if (magic != 0x52494646) { // 'RIFF'
      throw runtime_error(phosg::string_printf("unknown file format: %08" PRIX32, magic.load()));
    }
  }

  phosg::le_uint32_t file_size;
  phosg::freadx(f, &file_size, sizeof(uint32_t));

  WAVContents contents;
  WAVEHeader wav;
  wav.wave_magic = 0;
  for (;;) {
    RIFFChunkHeader chunk_header;
    phosg::freadx(f, &chunk_header, sizeof(RIFFChunkHeader));

    if (chunk_header.magic == 0x45564157) { // 'WAVE'
      memcpy(&wav, &chunk_header, sizeof(RIFFChunkHeader));
      phosg::freadx(f, reinterpret_cast<uint8_t*>(&wav) + sizeof(RIFFChunkHeader), sizeof(WAVEHeader) - sizeof(RIFFChunkHeader));

      if (wav.wave_magic != 0x45564157) { // 'WAVE'
        throw runtime_error(phosg::string_printf("sound has incorrect wave_magic (%" PRIX32 ")", wav.wave_magic.load()));
      }
      if (wav.fmt_magic != 0x20746D66) { // 'fmt '
        throw runtime_error(phosg::string_printf("sound has incorrect fmt_magic (%" PRIX32 ")", wav.fmt_magic.load()));
      }
      // We only support mono and stereo files for now
      if (wav.num_channels > 2) {
        throw runtime_error(phosg::string_printf("sound has too many channels (%hu)", wav.num_channels.load()));
      }

      contents.sample_rate = wav.sample_rate;
      contents.num_channels = wav.num_channels;
    } else if (chunk_header.magic == 0x6C706D73) { // 'smpl'
      if (wav.wave_magic == 0) {
        throw runtime_error("smpl chunk is before WAVE chunk");
      }

      const string data = phosg::freadx(f, chunk_header.size);
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
        // Convert the byte offsets to sample offsets
        contents_loop.start = header_loop->start / (wav.bits_per_sample >> 3);
        contents_loop.end = header_loop->end / (wav.bits_per_sample >> 3);
        contents_loop.type = header_loop->type;
      }
    } else if (chunk_header.magic == 0x61746164) { // 'data'
      if (wav.wave_magic == 0) {
        throw runtime_error("data chunk is before WAVE chunk");
      }

      contents.samples.resize((8 * chunk_header.size) / wav.bits_per_sample);

      // 32-bit float
      if ((wav.format == 3) && (wav.bits_per_sample == 32)) {
        phosg::freadx(f, contents.samples.data(), contents.samples.size() * sizeof(float));

        // 16-bit signed int
      } else if ((wav.format == 1) && (wav.bits_per_sample == 16)) {
        vector<int16_t> int_samples(contents.samples.size());
        phosg::freadx(f, int_samples.data(), int_samples.size() * sizeof(int16_t));
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
        phosg::freadx(f, int_samples.data(), int_samples.size() * sizeof(uint8_t));
        for (size_t x = 0; x < int_samples.size(); x++) {
          contents.samples[x] = (static_cast<float>(int_samples[x]) / 128.0f) - 1.0f;
        }
      } else {
        throw runtime_error(phosg::string_printf(
            "sample width is not supported (format=%hu, bits_per_sample=%hu)",
            wav.format.load(), wav.bits_per_sample.load()));
      }

      break;
    } else {
      fseek(f, chunk_header.size, SEEK_CUR);
    }
  }

  return contents;
}

} // namespace phosg_audio
