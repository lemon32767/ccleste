# ccleste

This is a C source port of the [original celeste (Celeste classic)](https://www.lexaloffle.com/bbs/?tid=2145) for the PICO-8.

The actual game is in celeste.c, all other files are but the 'frontend' to provide graphics and (eventually) audio output, using [allegro5](https://liballeg.org/).

Early work in progress.

![screenshot](https://raw.githubusercontent.com/lemon-sherbet/ccleste/master/screenshot.png)

## Known Issues/Compatibility

As of the time of writing, the game boots and is beatable, however:
	
	- Palette swapping is not yet complete, a rewrite to how sprites are handled will be done soon.
	- Audio isn't implemented at all yet.
	- Only keyboard XC+arrow keys control.
