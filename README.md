# ccleste

![screenshot](https://raw.githubusercontent.com/lemon-sherbet/ccleste/master/screenshot.png)

This is a C source port of the [original celeste (Celeste classic)](https://www.lexaloffle.com/bbs/?tid=2145) for the PICO-8, designed to be portable. PC and 3DS are the main supported platforms.

Go to [the releases tab](https://github.com/lemon-sherbet/ccleste/releases) for the latest pre-built binaries.

celeste.c + celeste.h is where the game code is, translated from the pico 8 lua code by hand.
These files don't depend on anything other than the c standard library and don't perform any allocations (it uses its own internal global state).

sdl12main.c provides a "frontend" written in SDL1.2 (plus SDL mixer) which implements graphics and audio output. It can be compiled on unix-like platforms by running
```
make
```

3DS is also a supported platform. Compile to 3DS with:
```
make -f Makefile.3ds    # this will generate ccleste.3dsx
```
You will need devkitPro with these dkp packages installed:
```
3ds-sdl
3ds-sdl_mixer
libctru
devkitARM
```

# Controls

|PC                |3DS                |Action      |
|:----------------:|:-----------------:|-----------:|
|LEFT              |LEFT               | Move left  |
|RIGHT             |RIGHT              | Move right |
|DOWN              |DOWN               | Look down  |
|UP                |UP                 | Look up    |
|Z/C               |A                  | Jump       |
|X/V               |B/X                | Dash       |
|ENTER             |START              | Pause      |
|F                 |Y                  | Fullscreen |
|ESC               |n/a                | Quit       |
|SHIFT+D           |SELECT+L           | Load state |
|SHIFT+S           |SELECT+R           | Save state |
|Hold SHIFT+ENTER+R|Hold SELECT+START+Y| Reset      |

# credits

Sound wave files are taken from [https://github.com/JeffRuLz/Celeste-Classic-GBA/tree/master/maxmod_data](https://github.com/JeffRuLz/Celeste-Classic-GBA/tree/master/maxmod_data),
music ogg files were obtained by converting the .wav dumps from pico 8, which I did using audacity & ffmpeg.

All credit for the original games goes to the original developers (Matt Thorson & Noel Berry).
