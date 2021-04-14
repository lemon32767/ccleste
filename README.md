# ccleste

![screenshot](https://raw.githubusercontent.com/lemon-sherbet/ccleste/master/screenshot.png)

This is a C source port of the [original celeste (Celeste classic)](https://www.lexaloffle.com/bbs/?tid=2145) for the PICO-8, designed to be portable. PC and 3DS are the main supported platforms, though other people are [maintaining ports to other platforms](https://github.com/lemon32767/ccleste/network/members).

Go to [the releases tab](https://github.com/lemon-sherbet/ccleste/releases) for the latest pre-built binaries.

An experimental web port is also available [here](https://lemon32767.github.io/ccleste.html).

celeste.c + celeste.h is where the game code is, translated from the pico 8 lua code by hand.
These files don't depend on anything other than the c standard library and don't perform any allocations (it uses its own internal global state).

sdl12main.c provides a "frontend" written in SDL (plus SDL mixer) which implements graphics and audio output. It can be compiled on unix-like platforms by running
```
make
```
By default, SDL2 will be used. Read the Makefile for more information.

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

|PC                |3DS                |Action              |
|:----------------:|:-----------------:|-------------------:|
|LEFT              |LEFT               | Move left          |
|RIGHT             |RIGHT              | Move right         |
|DOWN              |DOWN               | Look down          |
|UP                |UP                 | Look up            |
|Z/C               |A                  | Jump               |
|X/V               |B/X                | Dash               |
|ESCAPE            |START              | Pause              |
|E                 |L+R                | Toggle screenshake |
|SHIFT+D           |Y+L                | Load state         |
|SHIFT+S           |Y+R                | Save state         |
|Hold F9           |Hold SELECT+START+Y| Reset              |
|F11               |SELECT             | Fullscreen         |

Controller input is also supported on PC (SDL2 ver) and web version. The controller must be plugged in when opening the game.
The default mappings are: jump with A and dash with B (xbox360 controller layout), move with d-pad or the left stick, pause with start, save/load state with left/right shoulder, exit with guide (logo button).
You can change these mappings by modifying the `ccleste-input-cfg.txt` file that will be created when you first run the game.

You can make the game start up in fullscreen by setting the environment variable `CCLESTE_START_FULLSCREEN` to "1", or by creating the file `ccleste-start-fullscreen.txt` in the game directory (contents don't matter).

# TAS playback and the fixed point question

In order to playback a TAS, specify it as the first argument to the program when running it. On Windows you can drag the TAS file to the .exe to do this.
The format for the TAS should be a text file that looks like "0,0,3,5,1,34,0,", where each number is the input bitfield and each frame is ended by a comma.
The inputs in the TAS should start in the first loading frame of 100m (neglecting the title screen). When playing back a TAS the starting RNG seed will always be the same.

Most other Celeste Classic ports use floating point numbers, but PICO-8 actually uses 16.16 fixed point numbers.
For casual play and RTA speedrunning, the discrepancies are minor enough to be essentially negligible, however with TASing it might make a difference.
Defining the preprocessor macro `CELESTE_P8_FIXEDP` when compiling celeste.c will use a bunch of preprocessor hacks to replace the float type for all the
code of that file with a fixed point type that matches that of PICO-8. The use of this preprocessor macro requires compiling celeste.c with a C++ compiler, however (but not linking with the C++ standard library).

Using make you can compile this fixed point version with `make USE_FIXEDP=1`.

When playing back TASes made with other tools that work under the assumption of ideal RNG for balloons (since their hitbox depends on that), you can ensure that they do not desync by
defining the preprocessor macro `CELESTE_P8_HACKED_BALLOONS`, which will make balloons static and always expand their hitbox to their full range.

Using make you can turn on this feature with `make HACKED_BALLOONS=1`.

You can combine both of these with `make HACKED_BALLOONS=1 USE_FIXEDP=1`.

# credits

Sound wave files are taken from [https://github.com/JeffRuLz/Celeste-Classic-GBA/tree/master/maxmod_data](https://github.com/JeffRuLz/Celeste-Classic-GBA/tree/master/maxmod_data),
music ogg files were obtained by converting the .wav dumps from pico 8, which I did using audacity & ffmpeg.

All credit for the original game goes to the original developers (Maddy Thorson & Noel Berry).
