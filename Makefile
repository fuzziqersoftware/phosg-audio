OBJECTS=Constants.o File.o Capture.o Sound.o Stream.o
CXX=g++ -fPIC
CXXFLAGS=-std=c++14 -g -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -Wall -Werror
LDFLAGS=-g -std=c++14 -lstdc++

ALL_TARGETS=libphosg-audio.a

ifeq ($(OS),Windows_NT)
	# INSTALL_DIR not set because we don't support installing on windows. also,
	# we expect gcc and whatnot to be on PATH; probably the user has to set this
	# up manually but w/e
	CXXFLAGS +=  -DWINDOWS -D__USE_MINGW_ANSI_STDIO=1

	RM=del /S
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
