#!/bin/sh

WSYSROOT=/usr/i686-w64-mingw32/sys-root/mingw/

C () {
  echo ">>" $@
 
  if $@; then :; else
    echo Stop.
    exit 127
  fi
}
C mkdir -p win-build
C cd win-build
ZIGFLAGS="-O2 -g -target i386-windows-gnu -I $WSYSROOT/include/ -I $WSYSROOT/include/SDL/ -L $WSYSROOT/lib/ -Wno-ignored-attributes"

#normal
C zig cc  $ZIGFLAGS -c ../celeste.c -o celeste.o
C zig cc  $ZIGFLAGS -lSDLmain -lSDL  -lSDL_mixer celeste.o ../sdl12main.c -o ccleste.exe

#fixed point
C zig c++ $ZIGFLAGS -DCELESTE_P8_FIXEDP -c -xc++ ../celeste.c -o celeste-fixedp.o
C zig cc  $ZIGFLAGS -lSDLmain -lSDL  -lSDL_mixer celeste-fixedp.o ../sdl12main.c -o ccleste-fixedp.exe

#fixed point + balloon hack
C zig c++ $ZIGFLAGS -DCELESTE_P8_FIXEDP -DCELESTE_P8_HACKED_BALLOONS -c -xc++ ../celeste.c -o celeste-fixedp-balloonhack.o
C zig cc  $ZIGFLAGS -lSDLmain -lSDL  -lSDL_mixer celeste-fixedp-balloonhack.o ../sdl12main.c -o ccleste-fixedp-balloonhack.exe

C rm -rf zig-cache *.o stdout.txt
C cp $WSYSROOT/bin/libgcc_s_*.dll .
C cp $WSYSROOT/bin/libogg*.dll .
C cp $WSYSROOT/bin/libwinpthread*.dll .
C cp $WSYSROOT/bin/libvorbis*.dll .
C cp $WSYSROOT/bin/SDL.dll .
C cp $WSYSROOT/bin/SDL_mixer.dll .
C cp -r ../data/ .
C zip -9r ccleste-win.zip .
C mv ccleste-win.zip ..
