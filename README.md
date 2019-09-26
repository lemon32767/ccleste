# ccleste

This is a C source port of the [original celeste (Celeste classic)](https://www.lexaloffle.com/bbs/?tid=2145) for the PICO-8.

The actual game is in celeste.c, all other files are but the 'frontend' to provide graphics and (eventually) audio output, using [allegro5](https://liballeg.org/).

Early work in progress.

![screenshot](https://raw.githubusercontent.com/lemon-sherbet/ccleste/master/screenshot.png)

## Known Issues/Compatibility

As of the time of writing, the game boots and works for the most part, however:

	- The title screen doesn't display anything other than the text.
	- Physics are broken: for some reason jumps last longer and go further, holding towards a wall doesn't slide you down, etc...
	- There's several graphical glitches, such as hair not working, some BG tiles not displaying, palette swaps not being implemented, weird sprites displaying in the death animation, etc.
	- The levels completely break from 1700m onwards
	- Audio isn't implemented at all yet.
	- Only keyboard XC+arrow keys control.
	- Trig functions are not correctly implemented.
