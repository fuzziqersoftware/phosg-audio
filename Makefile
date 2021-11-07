OBJECTS=Constants.o File.o Convert.o Capture.o Sound.o Stream.o FourierTransform.o
CXX=g++ -fPIC
CXXFLAGS=-std=c++20 -g -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -Wall -Werror
LDFLAGS=-g -std=c++20 -lstdc++

ALL_TARGETS=libphosg-audio.a

ifeq ($(OS),Windows_NT)
	# setting up the windows build is very tedious. sorry about this, but I don't
	# write code for windows usually and I don't use visual studio, so most of the
	# default development path stuff doesn't work here.
	#
	# to build for windows, download openal-soft and phosg and extract them
	# somewhere. set the following paths to the places where you extracted them -
	# OPENAL_SOFT_DIR should point to a directory that has an include/
	# subdirectory, but PHOSG_DIR should point to the enclosing directory (that
	# has a phosg/ subdirectory).
	OPENAL_SOFT_DIR = X:\\phosg-audio\\windows-deps\\openal-soft-1.20.1-bin
	PHOSG_DIR = X:\\
	CXXFLAGS +=  -DWINDOWS -D__USE_MINGW_ANSI_STDIO=1 -I$(OPENAL_SOFT_DIR)\\include -I$(PHOSG_DIR)

	RM=rm -rf
	EXE_EXTENSION=.exe
else
	RM=rm -rf
	EXE_EXTENSION=
	ALL_TARGETS +=  audiocat$(EXE_EXTENSION)

	ifeq ($(shell uname -s),Darwin)
		INSTALL_DIR=/opt/local
		CXXFLAGS += -I/opt/local/include -DMACOSX -mmacosx-version-min=10.11
		LDFLAGS += -L/opt/local/lib -mmacosx-version-min=10.11 
		LIBS = -lphosg -lpthread -framework OpenAL
	else
		INSTALL_DIR=/usr/local
		CXXFLAGS += -I/usr/local/include -DLINUX
		LDFLAGS += -L/opt/local/lib 
		LIBS = -lphosg -lpthread -lopenal
	endif
endif

all: $(ALL_TARGETS)

install: libphosg-audio.a
	mkdir -p $(INSTALL_DIR)/include/phosg-audio
	cp libphosg-audio.a $(INSTALL_DIR)/lib/
	cp -r *.hh $(INSTALL_DIR)/include/phosg-audio/
	cp audiocat $(INSTALL_DIR)/bin/

libphosg-audio.a: $(OBJECTS)
	$(RM) libphosg-audio.a
	ar rcs libphosg-audio.a $(OBJECTS)

audiocat$(EXE_EXTENSION): $(OBJECTS) Audiocat.o
	$(CXX) $(LDFLAGS) $^ $(LIBS) -o $@

clean:
	$(RM) *.dSYM *.o gmon.out libphosg-audio.a audiocat$(EXE_EXTENSION)

.PHONY: clean
