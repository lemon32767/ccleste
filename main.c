#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "celeste.h"
#include "emulator.h"
#include "lemon.h"

static ALLEGRO_DISPLAY* display = NULL;
static ALLEGRO_EVENT_QUEUE* event_queue = NULL;

int gGameRun = 1;
int gWidth = 128*4, gHeight = 128*4;

void cleanup(void);
void process_event(ALLEGRO_EVENT* ev);

int main() {
	/* section $INIT */

	al_init();
	al_init_image_addon();
	al_install_keyboard();

	//create display
	al_set_new_display_flags(ALLEGRO_WINDOWED);
	al_set_new_display_option(ALLEGRO_VSYNC, 1, ALLEGRO_SUGGEST);
	display = al_create_display(gWidth, gHeight);
	strong_assert(display != NULL);
	int has_vsync = al_get_display_option(display, ALLEGRO_VSYNC) != 2;

	//create event queue
	event_queue = al_create_event_queue();
	strong_assert(event_queue != NULL);
	al_register_event_source(event_queue, al_get_keyboard_event_source());

	//change working dir to assets dir, which is the same dir as the one where the executable is
	ALLEGRO_PATH* dir = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
	if (dir) { //if it fails, try to keep going
		#ifdef _WIN32
		_chdir(al_path_cstr(dir, '\\'));
		#else
		chdir(al_path_cstr(dir, '/'));
		#endif
		al_destroy_path(dir);
	}

	//init emulator (load assets, set up function pointers etc)
	InitEmu();

	//init celeste
	Celeste_P8_init();

	int oddFrame = 0;

	ALLEGRO_BITMAP* screen = al_create_bitmap(128, 128);
	strong_assert(screen != NULL);

	while (gGameRun) {
		oddFrame = !oddFrame;
		if (oddFrame) goto end_frame; // we only update every other frame because  pico 8 runs at 30fps while vsync will be 60fps in 99% of platforms

		ALLEGRO_EVENT ev;
		while (al_get_next_event(event_queue, &ev)) {
			process_event(&ev);
		}

		al_set_target_bitmap(screen);

		/* section $UPDATE */
		EmuUpdate();

		/* section $DRAW */
		EmuDraw();

		end_frame:
		al_set_target_bitmap(al_get_backbuffer(display));
		al_draw_scaled_bitmap(screen, 0,0,128, 128, 0,0,gWidth, gHeight, 0);
		al_flip_display(); //waits for vsync if possible
		if (!has_vsync) {
		}
	}

	al_destroy_bitmap(screen);

	cleanup();
}

void cleanup() {
	/* section $CLEANUP */
	DeinitEmu();
	if (display) al_destroy_display(display);
	if (event_queue) al_destroy_event_queue(event_queue);
}

void process_event(ALLEGRO_EVENT* ev) {
	/* section $EVENTS */
	switch (ev->type) {
		case ALLEGRO_EVENT_KEY_DOWN:
			if (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE) gGameRun = 0;
			else {
				InputEvKeyDown(ev->keyboard.keycode);
			}
			break;
		case ALLEGRO_EVENT_KEY_UP:
			InputEvKeyUp(ev->keyboard.keycode);
			break;
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			gGameRun = 0;
			break;
	}
}