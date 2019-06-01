#pragma once

#ifndef WINDOWS
#ifdef MACOSX
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#endif

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include <vector>



#ifndef WINDOWS

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

#else

#define AL_FORMAT_MONO_FLOAT32    0
#define AL_FORMAT_STEREO_FLOAT32  1
#define AL_FORMAT_MONO8           2
#define AL_FORMAT_STEREO8         3
#define AL_FORMAT_MONO16          4
#define AL_FORMAT_STEREO16        5

#endif

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
