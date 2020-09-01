#!/bin/sh
CFLAGS="-Wall -O2"
EMFLAGS="-s ALLOW_MEMORY_GROWTH=1 -s USE_SDL=2 -s USE_SDL_MIXER=2 -s USE_OGG=1"
OUTDIR="em-build/"

mkdir -p $OUTDIR
emcc $CFLAGS $EMFLAGS sdl12main.c  -c  -o $OUTDIR/sdl12main.wasm
emcc $CFLAGS $EMFLAGS celeste.c  -c  -o $OUTDIR/celeste.wasm
emcc $EMFLAGS --shell-file emscripten-shell.html --preload-file data/ $OUTDIR/sdl12main.wasm $OUTDIR/celeste.wasm -o $OUTDIR/ccleste.html
