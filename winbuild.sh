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
C zig cc -target i386-windows-gnu -I $WSYSROOT/include/ -I $WSYSROOT/include/SDL/ \
  -L $WSYSROOT/lib/ -lSDLmain -lSDL -lSDL_mixer ../sdl12main.c ../celeste.c -occleste.exe
C rm -r zig-cache
C cp $WSYSROOT/bin/libgcc_s_*.dll .
C cp $WSYSROOT/bin/libogg*.dll .
C cp $WSYSROOT/bin/libwinpthread*.dll .
C cp $WSYSROOT/bin/libvorbis*.dll .
C cp $WSYSROOT/bin/SDL.dll .
C cp $WSYSROOT/bin/SDL_mixer.dll .
C cp -r ../data/ .
C zip -9r ccleste-win.zip .
C mv ccleste-win.zip ..
