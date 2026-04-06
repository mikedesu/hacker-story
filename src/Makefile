
CXX = clang++


WFLAGS = -Wall -Werror -std=c++17


MAIN_C     = main.cpp
LIBDRAW_C  = libdraw.cpp
LIBINPUT_C = libinput.cpp
GAME_SOURCES = $(MAIN_C) $(LIBDRAW_C) $(LIBINPUT_C)


RAYLIB_INCLUDES = -I/usr/local/include
RAYLIB_LIBS     = -L/usr/local/lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11


CFLAGS   ?=
CXXFLAGS ?=


# Web build setup
WEB_CC      = emcc
WEB_INCLUDES = -I. -I/usr/local/include \
               -I/home/darkmage/src/emsdk/upstream/emscripten/cache/sysroot/include
WEB_LINKS   = -L. -L/home/darkmage/src/raylib/src -l:libraylib.web.a
EMCC_OPTIONS = -s USE_GLFW=3 -s EXPORTED_RUNTIME_METHODS=ccall -s ALLOW_MEMORY_GROWTH
SHELL_FILE   = --shell-file minshell.html
PRELOAD_FILES = --preload-file ./img --preload-file ./audio --preload-file ./shaders
WEB_OPTIONS  = -DPLATFORM_WEB -DWEB


# Phony targets
.PHONY: all clean docs docs-clean tests web


all: game tests


# ── Desktop build ──────────────────────────────────────────────────────────────

game: main.o libdraw.o libinput.o
	$(CXX) $(WFLAGS) $(CXXFLAGS) $(CFLAGS) $^ $(RAYLIB_LIBS) -o $@

main.o: main.cpp
	$(CXX) $(WFLAGS) $(CXXFLAGS) $(CFLAGS) $(RAYLIB_INCLUDES) -c $< -o $@

libdraw.o: libdraw.cpp
	$(CXX) $(WFLAGS) $(CXXFLAGS) $(CFLAGS) $(RAYLIB_INCLUDES) -c $< -o $@

libinput.o: libinput.cpp
	$(CXX) $(WFLAGS) $(CXXFLAGS) $(CFLAGS) -c $< -o $@


# ── Web build ──────────────────────────────────────────────────────────────────

web: index.html

index.html: $(GAME_SOURCES)
	$(WEB_CC) -o $@ $(GAME_SOURCES) \
		$(WEB_INCLUDES) $(WEB_LINKS) \
		$(EMCC_OPTIONS) $(SHELL_FILE) $(PRELOAD_FILES) \
		$(WEB_OPTIONS) $(CFLAGS) $(CXXFLAGS)


# ── Unit tests ─────────────────────────────────────────────────────────────────

FAST_TEST_HEADERS = unit_test.h \
	test_suites/test_gamestate_lifecycle.h \
	test_suites/test_character_creation.h \
	test_suites/test_news_events.h \
	test_suites/test_gameplay_decisions.h

tests.cpp: $(FAST_TEST_HEADERS)
	cxxtestgen --error-printer -o tests.cpp $(FAST_TEST_HEADERS)

tests: tests.cpp libinput.o
	g++ -o tests tests.cpp libinput.o \
		$(RAYLIB_INCLUDES) -L/usr/local/lib -lraylib \
		-I. $(CXXFLAGS) $(CFLAGS)


# ── Documentation ─────────────────────────────────────────────────────────────

docs:
	doxygen Doxyfile

docs-clean:
	rm -rf docs/api


# ── Clean ──────────────────────────────────────────────────────────────────────

clean:
	rm -rfv game *.o *.gch \
		index.html index.js index.wasm index.data \
		tests tests.cpp
