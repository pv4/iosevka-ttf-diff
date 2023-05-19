#
# src/makefile
#

VERSION = 0.2.dev

BIN = iosevka-ttf-diff

CC = cc

LDFLAGS_OPT = \
	-Wl,-O1,-s,--as-needed,--no-keep-memory
CFLAGS_OPT = \
	$(LDFLAGS_OPT) \
	-mtune=native \
	-Os -s -fomit-frame-pointer \
	-pipe --param=ggc-min-expand=25 --param=ggc-min-heapsize=131072

DEP_HARFBUZZ = harfbuzz
DEP_HARFBUZZ_VERSION = 3.2
DEP_HARFBUZZ_CFLAGS = $(shell pkg-config --cflags $(DEP_HARFBUZZ))
DEP_HARFBUZZ_LDFLAGS = $(shell pkg-config --libs $(DEP_HARFBUZZ))
DEP_FREETYPE = freetype2
DEP_FREETYPE_VERSION = 24.1
DEP_FREETYPE_CFLAGS = $(shell pkg-config --cflags $(DEP_FREETYPE))
DEP_FREETYPE_LDFLAGS = $(shell pkg-config --libs $(DEP_FREETYPE))
DEP_LIBPNG = libpng
DEP_LIBPNG_VERSION = 1.6
DEP_LIBPNG_CFLAGS = $(shell pkg-config --cflags $(DEP_LIBPNG))
DEP_LIBPNG_LDFLAGS = $(shell pkg-config --libs $(DEP_LIBPNG))

CFLAGS_DEP = $(DEP_HARFBUZZ_CFLAGS) $(DEP_FREETYPE_CFLAGS) $(DEP_LIBPNG_CFLAGS)
LDFLAGS_DEP = $(DEP_HARFBUZZ_LDFLAGS) $(DEP_FREETYPE_LDFLAGS) $(DEP_LIBPNG_LDFLAGS)

CFLAGS = -Wall -Wno-parentheses $(CFLAGS_OPT) $(CFLAGS_DEP)
LDFLAGS = $(LDFLAGS_OPT) $(LDFLAGS_DEP)

SOURCES = \
	alloc.c \
	compare_args.c \
	compare_main.c \
	error.c \
	font.c \
	glyph.c \
	gbox.c \
	gdumper.c \
	main.c \
	map_main.c \
	png.c \
	vmap.c

$(BIN): $(SOURCES)
	pkg-config --print-errors --atleast-version $(DEP_HARFBUZZ_VERSION) $(DEP_HARFBUZZ)
	pkg-config --print-errors --atleast-version $(DEP_FREETYPE_VERSION) $(DEP_FREETYPE)
	pkg-config --print-errors --atleast-version $(DEP_LIBPNG_VERSION) $(DEP_LIBPNG)
	$(CC) $(CFLAGS) -DVERSION="\"$(VERSION)\"" -o $@ $(SOURCES) $(LDFLAGS)

all: $(BIN)

clean:
	rm -f iosevka-ttf-diff

install: all
	printf "%s\n" \
		"Just copy $(BIN) wherever you want." \
		"Alternatively you can run it directly from ./src/ directory."
