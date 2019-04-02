OBJECTS=Constants.o File.o Capture.o Sound.o Stream.o
CXX=g++ -fPIC
CXXFLAGS=-std=c++14 -g -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -Wall -Werror
LDFLAGS=-g -std=c++14 -lstdc++

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

all: libphosg-audio.a audiocat

install: libphosg-audio.a
	mkdir -p $(INSTALL_DIR)/include/phosg-audio
	cp libphosg-audio.a $(INSTALL_DIR)/lib/
	cp -r *.hh $(INSTALL_DIR)/include/phosg-audio/
	cp audiocat $(INSTALL_DIR)/bin/

libphosg-audio.a: $(OBJECTS)
	rm -f libphosg-audio.a
	ar rcs libphosg-audio.a $(OBJECTS)

audiocat: $(OBJECTS) Audiocat.o
	$(CXX) $(LDFLAGS) $^ $(LIBS) -o $@

clean:
	rm -rf *.dSYM *.o gmon.out libphosg-audio.a audiocat

.PHONY: clean
