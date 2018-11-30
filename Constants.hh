#pragma once

#ifdef MACOSX
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include <stdint.h>
#include <sys/types.h>

#include <vector>


void init_al();
void exit_al();

const char* al_err_str(ALenum err);

#define __al_check_error(file,line) \
  do { \
    for (ALenum err = alGetError(); err != AL_NO_ERROR; err = alGetError()) \
      fprintf(stderr, "AL error %s at %s:%d\n", al_err_str(err), file, line); \
  } while(0)

#define al_check_error() \
    __al_check_error(__FILE__, __LINE__)

bool is_16bit(int format);
bool is_32bit(int format);
bool is_stereo(int format);
size_t bytes_per_sample(int format);

const char* name_for_format(int format);
int format_for_name(const char* format);

uint8_t note_for_name(const char* name);
const char* name_for_note(uint8_t note);

double frequency_for_note(uint8_t note);

void byteswap_samples(void* buffer, size_t sample_count, int format);
std::vector<float> convert_samples_to_float(const std::vector<int16_t>&);
std::vector<int16_t> convert_samples_to_int(const std::vector<float>&);
