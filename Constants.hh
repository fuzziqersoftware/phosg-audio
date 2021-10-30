#pragma once

#ifdef MACOSX
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include <vector>
#include <set>
#include <string>



std::set<std::string> list_audio_device_names();
std::string get_current_audio_device_name();

void init_al(const char* device_name = NULL);
void exit_al();

const char* al_err_str(ALenum err);

#define __al_check_error(file,line) \
  do { \
    for (ALenum err = alGetError(); err != AL_NO_ERROR; err = alGetError()) { \
      fprintf(stderr, "AL error %s at %s:%d\n", al_err_str(err), file, line); \
    } \
  } while(0)

#define al_check_error() \
    __al_check_error(__FILE__, __LINE__)

bool is_16bit(int format);
bool is_32bit(int format);
bool is_stereo(int format);
size_t bytes_per_sample(int format);
size_t bytes_per_frame(int format);

const char* name_for_format(int format);
int format_for_name(const char* format);

uint8_t note_for_name(const char* name);
const char* name_for_note(uint8_t note);

double frequency_for_note(uint8_t note);
