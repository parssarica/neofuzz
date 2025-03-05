CC           = cc
SOURCES      = neofuzz.c utils.c fuzzop.c sds.c
LIBS         = -lcurl -lpthread

CFLAGS_BASE   = -Wall -Wextra -pedantic -I/usr/local/include -L/usr/local/lib
DEBUG_FLAGS   = -ggdb3 -O0 -DFORTIFY_SOURCE=3
RELEASE_FLAGS = -O3
SANITIZER_ASAN= -fsanitize=address -fstack-protector
SANITIZER_TSAN= -fsanitize=thread -fstack-protector
PROFILE_FLAGS = -pg -DFORTIFY_SOURCE=3

TARGET = neofuzz

PREFIX   ?= /usr/local
BINDIR   := $(PREFIX)/bin

OS       := $(shell uname)
ifeq ($(OS),Darwin)
    INSTALL = install -c
else
    INSTALL = install
endif

all: release

release:
	$(CC) $(CFLAGS_BASE) $(RELEASE_FLAGS) $(SOURCES) $(LIBS) -o $(TARGET)

debug:
	$(CC) $(CFLAGS_BASE) $(DEBUG_FLAGS) $(SOURCES) $(LIBS) -o $(TARGET)

debugasan:
	$(CC) $(CFLAGS_BASE) $(DEBUG_FLAGS) $(SANITIZER_ASAN) $(SOURCES) $(LIBS) -o $(TARGET)

debugtsan:
	$(CC) $(CFLAGS_BASE) $(DEBUG_FLAGS) $(SANITIZER_TSAN) $(SOURCES) $(LIBS) -o $(TARGET)

debugprofile:
	$(CC) $(CFLAGS_BASE) $(DEBUG_FLAGS) $(PROFILE_FLAGS) $(SOURCES) $(LIBS) -o $(TARGET)

install: all
	@echo "Installing $(TARGET) to $(BINDIR)"
	$(INSTALL) -d $(BINDIR)
	$(INSTALL) -m 0755 $(TARGET) $(BINDIR)/$(TARGET)

uninstall:
	@echo "Removing $(TARGET) from $(BINDIR)"
	rm -f $(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all release debug debugasan debugtsan debugprofile install uninstall clean


