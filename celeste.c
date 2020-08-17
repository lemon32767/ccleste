/*
* This is where the actual celeste code sits.
* It is mostly a line by line port of the original lua code.
* Due to C limitations, modifications have to be made, mostly relating to static typing.
* The PICO-8 functions such as music() are used here preceded by Celeste_P8,
* so _init becomes Celeste_P8_init && music becomes P8music, etc 
*/

//TODO: USE 16BIT FIXED POINT INSTEAD OF DOUBLE LIKE IN PICO-8

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "celeste.h"

//i cant be bothered to put all function declarations in an appropiate place so ill just toss them all here:
static void PRELUDE(void);
static void PRELUDE_initclouds(void);
static void PRELUDE_initparticles(void);
static void title_screen(void);
static void load_room(int x, int y);
static void next_room(void);
static void psfx(int num);
static void restart_room(void);

#define bool Celeste_P8_bool_t
#define false 0
#define true 1

static float clamp(float val, float a, float b);
static float appr(float val, float target, float amount);
static int sign(float v);
static bool maybe(void);
static bool solid_at(int x,int y,int w,int h);
static bool ice_at(int x,int y,int w,int h);
static bool tile_flag_at(int x,int y,int w,int h,int flag);
static int tile_at(int x,int y);
static bool spikes_at(float x,float y,int w,int h,float xspd,float yspd);


//exported /imported functions
static callback_func_t Celeste_P8_call = NULL;

//exported
void Celeste_P8_set_call_func(callback_func_t func) {
	Celeste_P8_call = func;
}

///////

// https://github.com/lemon-sherbet/ccleste/issues/1
static float P8modulo(float a, float b) {
	return fmod(fmod(a, b) + b, b);
}
#define P8max fmax
#define P8abs fabsf
#define P8min fmin
#define P8flr floor
static inline float P8sin(float x) {
	return -sinf(x*6.2831853071796); //https://pico-8.fandom.com/wiki/Math
}
#define P8cos(x) (-P8sin((x)+0.25)) //cos(x) = sin(x+pi/2)
static inline float P8rnd(float max) {
	return Celeste_P8_call(CELESTE_P8_RND, max).f;
}
static inline void P8music(int track, int fade, int mask) {
	Celeste_P8_call(CELESTE_P8_MUSIC, track, fade, mask);
}
static inline void P8spr(int sprite, int x, int y, int cols, int rows, bool flipx, bool flipy) {
	Celeste_P8_call(CELESTE_P8_SPR, sprite, x, y, cols, rows, flipx, flipy);
}
static inline bool P8btn(int b) {
	return Celeste_P8_call(CELESTE_P8_BTN, b).i;
}
static inline void P8sfx(int id) {
	Celeste_P8_call(CELESTE_P8_SFX, id);
}
static inline void P8pal(int a, int b) {
	Celeste_P8_call(CELESTE_P8_PAL, a, b);
}
static inline void P8pal_reset() {
	Celeste_P8_call(CELESTE_P8_PAL_RESET);
}
static inline void P8circfill(int x, int y, float r, int c) {
	Celeste_P8_call(CELESTE_P8_CIRCFILL, x,y,r,c);
}
static inline void P8rectfill(int x, int y, int x2, int y2, int c) {
	Celeste_P8_call(CELESTE_P8_RECTFILL, x,y,x2,y2,c);
}
static inline void P8print(const char* str, int x, int y, int c) {
	Celeste_P8_call(CELESTE_P8_PRINT, str,x,y,c);
}
static inline void P8line(int x, int y, int x2, int y2, int c) {
	Celeste_P8_call(CELESTE_P8_LINE, x,y,x2,y2,c);
}
static inline int P8mget(int x, int y) {
	return Celeste_P8_call(CELESTE_P8_MGET, x,y).i;
}
static inline bool P8fget(int t, int f) {
	return Celeste_P8_call(CELESTE_P8_FGET, t,f).i;
}
static inline void P8camera(int x, int y) {
	Celeste_P8_call(CELESTE_P8_CAMERA, x, y);
}
static inline void P8map(int mx, int my, int tx, int ty, int mw, int mh, int mask) {
	Celeste_P8_call(CELESTE_P8_MAP, mx, my, tx, ty, mw, mh, mask);
}

#define MAX_OBJECTS 24
#define FRUIT_COUNT 30

// ~celeste~
// matt thorson + noel berry

// globals
//////////////

typedef struct {float x,y;} VEC;
typedef struct {int x,y;} VECI;

static VECI room = {.x=0,.y=0};
//static int num_objects = 0;
static int freeze = 0;
static int shake = 0;
static bool will_restart = false;
static int delay_restart = 0;
static bool got_fruit[FRUIT_COUNT] = {false};
static bool has_dashed = false;
static int sfx_timer = 0;
static bool has_key = false;
static bool pause_player = false;
static bool flash_bg = false;
static int music_timer = 0;

//these are originally implicit globals defined in title_screen()
static bool new_bg = false;
static int frames, seconds, minutes;
static int deaths, max_djump;
static bool start_game;
static int start_game_flash;


static int k_left = 0;
static int k_right = 1;
static int k_up = 2;
static int k_down = 3;
static int k_jump = 4;
static int k_dash = 5;

//with this X macro table thing we can define the properties that each object type has, in the original lua code these properties
//are inferred from the `types` table
#define OBJ_PROP_LIST() \
	/* TYPE        TILE   HAS INIT  HAS UPDATE  HAS DRAW  */\
	X(PLAYER,       -1,     Y,        Y,          Y)\
	X(PLAYER_SPAWN,  1,     Y,        Y,          Y)\
	X(SPRING,       18,     Y,        Y,          N)\
	X(BALLOON,      22,     Y,        Y,          Y)\
	X(SMOKE,        -1,     Y,        Y,          N)\
	X(PLATFORM,     -1,     Y,        Y,          Y)\
	X(FALL_FLOOR,   23,     Y,        Y,          Y)\
	X(FRUIT,        26,     Y,        Y,          N)\
	X(FLY_FRUIT,    28,     Y,        Y,          Y)\
	X(FAKE_WALL,    64,     N,        Y,          Y)\
	X(KEY,           8,     N,        Y,          N)\
	X(CHEST,        20,     Y,        Y,          N)\
	X(LIFEUP,       -1,     Y,        Y,          Y)\
	X(MESSAGE,      86,     N,        N,          Y)\
	X(BIG_CHEST,    96,     Y,        N,          Y)\
	X(ORB,          -1,     Y,        N,          Y)\
	X(FLAG,        118,     Y,        N,          Y)\
	X(ROOM_TITLE,   -1,     Y,        N,          Y)

typedef enum {
	#define X(t,...) OBJ_##t,
	OBJ_PROP_LIST()
	#undef X
	OBJTYPE_COUNT
} OBJTYPE;

// entry point //
/////////////////

static void PRELUDE() {
	//top-level init code has been moved into functions that are called here
	PRELUDE_initclouds();
	PRELUDE_initparticles();
}

void Celeste_P8_init() { //identifiers beginning with underscores are reserved in C
	if (!Celeste_P8_call) {
		fprintf(stderr, "Warning: Celeste_P8_call is NULL.. have you called Celeste_P8_set_call_func()?\n");
	}

	PRELUDE();

	title_screen();
}

static void title_screen() {
	for (int i = 0; i <= 29; i++)
		got_fruit[i] = false;
	frames=0;
	deaths=0;
	max_djump=1;
	start_game=false;
	start_game_flash=0;
	P8music(40,0,7);
   
	load_room(7,3);
}

static void begin_game() {
	frames=0;
	seconds=0;
	minutes=0;
	music_timer=0;
	start_game=false;
	P8music(0,0,7);
	load_room(0,0);
}

static int level_index() {
	return room.x%8+room.y*8;
}

static bool is_title() {
	return level_index()==31;
}

// effects //
/////////////

typedef struct {
	bool isLast;
	int x,y,spd,w;
} CLOUD;
static CLOUD clouds[17];
//top level init code has been moved into a function
static void PRELUDE_initclouds() {
	for (int i=0; i<=16; i++) {
		clouds[i] = (CLOUD){
			.isLast = i == 16,

			.x=P8rnd(128),
			.y=P8rnd(128),
			.spd=1+P8rnd(4),
			.w=32+P8rnd(32)
		};
	}
}

typedef struct {
	bool isLast, active;
	float x,y,s,spd,off,c,h,t;
	VEC spd2; //used by dead particles, moved from spd
} PARTICLE;
static PARTICLE particles[25];
int particle_count = 0;
static PARTICLE dead_particles[25];
int dead_particles_count = 0;
//top level init code has been moved into a function
static void PRELUDE_initparticles() {
	for (int i=0; i<=24; i++) {
		particles[i] = (PARTICLE){
			.isLast = i == 24,

			.x=P8rnd(128),
			.y=P8rnd(128),
			.s=0+P8flr(P8rnd(5)/4),
			.spd=0.25+P8rnd(5),
			.off=P8rnd(1),
			.c=6+P8flr(0.5+P8rnd(1))
		};
		dead_particles[i].isLast = i == 24;
	}
}

typedef struct {int x,y,w,h;} HITBOX;

typedef struct {
	bool isLast;
	float x,y,size;
} HAIR;

//OBJECT strucutre
typedef struct {
	bool active;

	//inherited
	int type;
	bool collideable, solids;
	float spr;
	bool flip_x, flip_y;
	float x, y;
	HITBOX hitbox;
	VEC spd;
	VEC rem;

	//player
	bool p_jump, p_dash;
	int grace, jbuffer, djump, dash_time, dash_effect_time;
	VEC dash_target;
	VEC dash_accel;
	float spr_off;
	bool was_on_ground;
	HAIR hair[5]; //also player_spawn

	//player_spawn
	int state, delay;
	VEC target;

	//spring
	int hide_in, hide_for;

	//balloon
	int timer;
	float offset, start;

	//fruit
	float off;

	//fly_fruit
	bool fly;
	float step;
	int sfx_delay;

	//lifeup
	int duration;
	float flash;

	//platform
	float last, dir;

	//message
	char* text;
	float index;
	VECI off2; //changed from off..

	//big chest
	PARTICLE particles[50];
	int particle_count;

	//flag
	int score;
	bool show;
} OBJ;

//OBJ function declarations fuckery
#define when_Y(x) static void x(OBJ* this);
#define when_N(x) static void* x = NULL;
#define CAT(a,b) a##b
#define X(name,t,has_init,has_update,has_draw) \
	CAT(when_,has_init)(name##_init)\
	CAT(when_,has_update)(name##_update)\
	CAT(when_,has_draw)(name##_draw)
OBJ_PROP_LIST()
#undef X

typedef void (*obj_callback_t)(OBJ*);

struct objprop {
	int tile;
	obj_callback_t init;
	obj_callback_t update;
	obj_callback_t draw;
};

static struct objprop OBJTYPE_prop(OBJTYPE t) {
	switch (t) {
		#define X(name,t,has_init,has_update,has_draw) \
			case OBJ_##name:\
				return (struct objprop) {\
					.tile = t,\
					.init = name##_init, \
					.update = name##_update, \
					.draw = name##_draw \
				};
		OBJ_PROP_LIST()
		#undef X
		default:;
	}
	struct objprop dummy = {0};
	return dummy;
}

static OBJ objects[MAX_OBJECTS] = {{.active = false}};

static void create_hair(OBJ* obj);
static void set_hair_color(int c);
static void draw_hair(OBJ* obj, int facing);
static void unset_hair_color(void);
static void kill_player(OBJ* obj);
static void break_fall_floor(OBJ* obj);
static void draw_time(float x, float y);
static OBJ* init_object(OBJTYPE type, float x, float y);
static void destroy_object(OBJ* obj);
static void draw_object(OBJ* obj);

//OBJECT FUNCTIONS MOVED HERE

static bool OBJ_is_solid(OBJ* obj, float ox, float oy);
static bool OBJ_is_ice(OBJ* obj, float ox, float oy);
static OBJ* OBJ_collide(OBJ* obj, OBJTYPE type, float ox, float oy);
static bool OBJ_check(OBJ* obj, OBJTYPE type, float ox, float oy);
static void OBJ_move(OBJ* obj, float ox, float oy);
static void OBJ_move_x(OBJ* obj, float amount, float start);
static void OBJ_move_y(OBJ* obj, float amount);

static bool OBJ_is_solid(OBJ* obj, float ox, float oy) {
	if (oy>0 && !OBJ_check(obj, OBJ_PLATFORM,ox,0) && OBJ_check(obj, OBJ_PLATFORM,ox,oy)) {
		return true;
	}
	return solid_at(obj->x+obj->hitbox.x+ox,obj->y+obj->hitbox.y+oy,obj->hitbox.w,obj->hitbox.h)
	 || OBJ_check(obj, OBJ_FALL_FLOOR,ox,oy)
	 || OBJ_check(obj, OBJ_FAKE_WALL,ox,oy);
}

static bool OBJ_is_ice(OBJ* obj, float ox, float oy) {
	return ice_at(obj->x+obj->hitbox.x+ox,obj->y+obj->hitbox.y+oy,obj->hitbox.w,obj->hitbox.h);
}

static OBJ* OBJ_collide(OBJ* obj, OBJTYPE type, float ox, float oy) {
	OBJ* other;
	for (int i=0; i < MAX_OBJECTS; i++) {
		other=&objects[i];
		if (other->active && other->type == type && other != obj && other->collideable &&
			other->x+other->hitbox.x+other->hitbox.w > obj->x+obj->hitbox.x+ox && 
			other->y+other->hitbox.y+other->hitbox.h > obj->y+obj->hitbox.y+oy &&
			other->x+other->hitbox.x < obj->x+obj->hitbox.x+obj->hitbox.w+ox && 
			other->y+other->hitbox.y < obj->y+obj->hitbox.y+obj->hitbox.h+oy) {
				return other;
		}
	}
	return NULL;
}

static bool OBJ_check(OBJ* obj, OBJTYPE type, float ox, float oy) {
	return OBJ_collide(obj, type,ox,oy) != NULL;
}

static void OBJ_move(OBJ* obj, float ox, float oy) {
	float amount;
	// [x] get move amount
	obj->rem.x += ox;
	amount = P8flr(obj->rem.x + 0.5);
	obj->rem.x -= amount;
	OBJ_move_x(obj, amount,0);
   
	// [y] get move amount
	obj->rem.y += oy;
	amount = P8flr(obj->rem.y + 0.5);
	obj->rem.y -= amount;
	OBJ_move_y(obj, amount);
}

static void OBJ_move_x(OBJ* obj, float amount, float start) {
	if (obj->solids) {
		int step = sign(amount);
		for (int i=start; i <= P8abs(amount); i++) {
			if (!OBJ_is_solid(obj, step,0)) {
				obj->x += step;
			} else {
				obj->spd.x = 0;
				obj->rem.x = 0;
				break;
			}
		}
	} else {
		obj->x += amount;
	}
}

static void OBJ_move_y(OBJ* obj, float amount) {
	if (obj->solids) {
		int step = sign(amount);
		for (int i=0; i <= P8abs(amount); i++) {
		 if (!OBJ_is_solid(obj,0,step)) {
				obj->y += step;
			} else {
				obj->spd.y = 0;
				obj->rem.y = 0;
				break;
			}
		}
	} else {
		obj->y += amount;
	}
}


// player entity //
///////////////////

static void PLAYER_init(OBJ* this) {
	this->p_jump=false;
	this->p_dash=false;
	this->grace=0;
	this->jbuffer=0;
	this->djump=max_djump;
	this->dash_time=0;
	this->dash_effect_time=0;
	this->dash_target=(VEC){.x=0,.y=0};
	this->dash_accel=(VEC){.x=0,.y=0};
	this->hitbox = (HITBOX){.x=1,.y=3,.w=6,.h=5};
	this->spr_off=0;
	this->was_on_ground=false;
	create_hair(this);
}
static void PLAYER_update(OBJ* this) {
	if (pause_player) return;
   
	int input = P8btn(k_right) ? 1 : (P8btn(k_left) ? -1 : 0);

	/*LEMON: instead of calling kill_player() below, we call it at the end of this function.. */
	/*       this is because if we spawn smoke particles after doing kill_player they might spawn */
	/*       in the slot where the player was... and at the end of this function there is code that changes */
	/*       the spr property, to do player animation, but this would change the sprite of this smoke particle */

	int do_kill_player = 0;
   
	// spikes collide
	if (spikes_at(this->x+this->hitbox.x,this->y+this->hitbox.y,this->hitbox.w,this->hitbox.h,this->spd.x,this->spd.y)) {
		do_kill_player = 1;
	}
	 
	// bottom death
	if (this->y>128) {
		do_kill_player = 1;
	}

	bool on_ground=OBJ_is_solid(this, 0,1);
	bool on_ice=OBJ_is_ice(this, 0,1);
   
	// smoke particles
	if (on_ground && !this->was_on_ground) {
		init_object(OBJ_SMOKE,this->x,this->y+4);
	}

	bool jump = P8btn(k_jump) && !this->p_jump;
	this->p_jump = P8btn(k_jump);
	if ((jump)) {
		this->jbuffer=4;
	} else if (this->jbuffer>0) {
		this->jbuffer-=1;
	}
   
	bool dash = P8btn(k_dash) && !this->p_dash;
	this->p_dash = P8btn(k_dash);
   
	if (on_ground) {
		this->grace=6;
		if (this->djump<max_djump) {
			psfx(54);
			this->djump=max_djump;
		}
	} else if (this->grace > 0) {
		this->grace-=1;
	}

	this->dash_effect_time -=1;
	if (this->dash_time > 0) {
		init_object(OBJ_SMOKE, this->x,this->y);
		this->dash_time-=1;
		this->spd.x=appr(this->spd.x,this->dash_target.x,this->dash_accel.x);
		this->spd.y=appr(this->spd.y,this->dash_target.y,this->dash_accel.y); 
	} else {

		// move
		int maxrun=1;
		float accel=0.6;
		float deccel=0.15;
		 
		if (!on_ground) {
			accel=0.4;
		} else if (on_ice) {
			accel=0.05;
			if (input==(this->flip_x ? -1 : 1)) {
				accel=0.05;
			}
		}

		if (P8abs(this->spd.x) > maxrun) {
			this->spd.x=appr(this->spd.x,sign(this->spd.x)*maxrun,deccel);
		} else {
			this->spd.x=appr(this->spd.x,input*maxrun,accel);
		}
		 
		//facing
		if (this->spd.x!=0) {
			this->flip_x=(this->spd.x<0);
		}

		// gravity
		float maxfall=2;
		float gravity=0.21;

		if (P8abs(this->spd.y) <= 0.15) {
			gravity*=0.5;
		}

		// wall slide
		if (input!=0 && OBJ_is_solid(this, input,0) && !OBJ_is_ice(this, input,0)) {
			maxfall=0.4;
			if (P8rnd(10)<2) {
				init_object(OBJ_SMOKE,this->x+input*6,this->y);
			}
		}

		if (!on_ground) {
			this->spd.y=appr(this->spd.y,maxfall,gravity);
		}

		// jump
		if (this->jbuffer>0) {
			if (this->grace>0) {
				// normal jump
				psfx(1);
				this->jbuffer=0;
				this->grace=0;
				this->spd.y=-2;
				init_object(OBJ_SMOKE,this->x,this->y+4);
			} else {
				// wall jump
				int wall_dir=(OBJ_is_solid(this, -3,0) ? -1 : (OBJ_is_solid(this, 3,0) ? 1 : 0));
				if (wall_dir!=0) {
					psfx(2);
					this->jbuffer=0;
					this->spd.y=-2;
					this->spd.x=-wall_dir*(maxrun+1);
					if (!OBJ_is_ice(this, wall_dir*3,0)) {
						init_object(OBJ_SMOKE,this->x+wall_dir*6,this->y);
					}
				}
			}
		}
		 
		// dash
		float d_full=5;
		float d_half=d_full*0.70710678118;
   
		if (this->djump>0 && dash) {
			init_object(OBJ_SMOKE,this->x,this->y);
			this->djump-=1;
			this->dash_time=4;
			has_dashed=true;
			this->dash_effect_time=10;
			int v_input=(P8btn(k_up) ? -1 : (P8btn(k_down) ? 1 : 0));
			if (input!=0) {
				if (v_input!=0) {
					this->spd.x=input*d_half;
					this->spd.y=v_input*d_half;
				} else {
					this->spd.x=input*d_full;
					this->spd.y=0;
				}
			} else if (v_input!=0) {
				this->spd.x=0;
				this->spd.y=v_input*d_full;
			} else {
				this->spd.x=(this->flip_x ? -1 : 1);
				this->spd.y=0;
			}
		
			psfx(3);
			freeze=2;
			shake=6;
			this->dash_target.x=2*sign(this->spd.x);
			this->dash_target.y=2*sign(this->spd.y);
			this->dash_accel.x=1.5;
			this->dash_accel.y=1.5;
			
			if (this->spd.y<0) {
				this->dash_target.y*=.75;
			}
			
			if (this->spd.y!=0) {
				this->dash_accel.x*=0.70710678118;
			}
			if (this->spd.x!=0) {
				this->dash_accel.y*=0.70710678118;		
			} else if (dash && this->djump<=0) {
				psfx(9);
				init_object(OBJ_SMOKE,this->x,this->y);
			}
		}
	}
   
	// animation
	this->spr_off+=0.25;
	if (!on_ground) {
		if (OBJ_is_solid(this, input,0)) {
			this->spr=5;
		} else {
			this->spr=3;
		}
	} else if (P8btn(k_down)) {
		this->spr=6;
	} else if (P8btn(k_up)) {
		this->spr=7;
	} else if ((this->spd.x==0) || (!P8btn(k_left) && !P8btn(k_right))) {
		this->spr=1;
	} else {
		this->spr=1+((int)this->spr_off)%4;
	}
   
	// next level
	if (this->y<-4 && level_index()<30) { next_room(); }
   
	// was on the ground
	this->was_on_ground=on_ground;
   
	if (do_kill_player) kill_player(this);
}
static void PLAYER_draw(OBJ* this) {
	// clamp in screen
	if (this->x<-1 || this->x>121) { 
		this->x=clamp(this->x,-1,121);
		this->spd.x=0;
	}
   
	set_hair_color(this->djump);
	draw_hair(this,this->flip_x ? -1 : 1);
	P8spr(this->spr,this->x,this->y,1,1,this->flip_x,this->flip_y);
	unset_hair_color();
}

static void psfx(int num) {
	if (sfx_timer<=0) {
		P8sfx(num);
	}
}

void create_hair(OBJ* obj) {
	/*obj->hair = {};*/
	for (int i=0;i<=4;i++) {
		obj->hair[i] = (HAIR) {.x=obj->x,.y=obj->y,.size=P8max(1,P8min(2,3-i)), .isLast = i == 4};
	}
}

static void set_hair_color(int djump) {
	P8pal(8,(djump==1 ? 8 : (djump==2 ?(7+P8flr(((int)(((float)frames)/3.0))%2)*4) : 12)));
}

static void draw_hair(OBJ* obj, int facing) {
	float last_x=obj->x+4-facing*2;
	float last_y=obj->y+(P8btn(k_down) ? 4 : 3);
	HAIR* h;
	int i = 0;
	while (!(h = &obj->hair[i++])->isLast) {
		h->x+=(last_x-h->x)/1.5;
		h->y+=(last_y+0.5-h->y)/1.5;
		P8circfill(h->x,h->y,h->size,8);
		last_x=h->x;
		last_y=h->y;
	}
}

static void unset_hair_color() {
	P8pal(8,8);
}

//player_spawn
static void PLAYER_SPAWN_init(OBJ* this) {
	P8sfx(4);
	this->spr=3;
	this->target.x=this->x;
	this->target.y=this->y;
	this->y=128;
	this->spd.y=-4;
	this->state=0;
	this->delay=0;
	this->solids=false;
	create_hair(this);
}
static void PLAYER_SPAWN_update(OBJ* this) {
	// jumping up
	if (this->state==0) {
		if (this->y < this->target.y+16) {
			this->state=1;
			this->delay=3;
		}
	// falling
	} else if (this->state==1) {
		this->spd.y+=0.5;
		if (this->spd.y>0 && this->delay>0) {
			this->spd.y=0;
			this->delay-=1;
		}
		if (this->spd.y>0 && this->y > this->target.y) {
			this->y=this->target.y;
			this->spd.x = this->spd.y=0;
			this->state=2;
			this->delay=5;
			shake=5;
			init_object(OBJ_SMOKE,this->x,this->y+4);
			P8sfx(5);
		}
	// landing
	} else if (this->state==2) {
		this->delay-=1;
		this->spr=6;
		if (this->delay<0) {
			//destroy_object(this);
			init_object(OBJ_PLAYER,this->x,this->y);
			destroy_object(this); //LEMON: reordererd cus otherewise player object would not be drwn for the first frame (update wasnt being called since it overwrote this PLAYER_SPAWN slot)
		}
	}
}
static void PLAYER_SPAWN_draw (OBJ* this) {
	set_hair_color(max_djump);
	draw_hair(this,1);
	P8spr(this->spr,this->x,this->y,1,1,this->flip_x,this->flip_y);
	unset_hair_color();
}

//spring
static void SPRING_init(OBJ* this) {
	this->hide_in=0;
	this->hide_for=0;
}
static void SPRING_update(OBJ* this) {
	if (this->hide_for>0) {
		this->hide_for-=1;
		if (this->hide_for<=0) {
			this->spr=18;
			this->delay=0;
		}
	} else if (this->spr==18) {
		OBJ* hit = OBJ_collide(this, OBJ_PLAYER,0,0);
		if (hit != NULL && hit->spd.y>=0) {
			this->spr=19;
			hit->y=this->y-4;
			hit->spd.x*=0.2;
			hit->spd.y=-3;
			hit->djump=max_djump;
			this->delay=10;
			init_object(OBJ_SMOKE,this->x,this->y);
			 
			// breakable below us
			OBJ* below=OBJ_collide(this, OBJ_FALL_FLOOR,0,1);
			if (below != NULL) {
				break_fall_floor(below);
			}
			 
			psfx(8);
		}
	} else if (this->delay>0) {
		this->delay-=1;
		if (this->delay<=0) { 
			this->spr=18;
		}
	}
	// begin hiding
	if (this->hide_in>0) {
		this->hide_in-=1;
		if (this->hide_in<=0) {
			this->hide_for=60;
			this->spr=0;
		}
	}
}

static void break_spring(OBJ* obj){
	obj->hide_in=15;
}

//balloon
static void BALLOON_init(OBJ* this) {
	this->offset=P8rnd(1);
	this->start=this->y;
	this->timer=0;
	this->hitbox=(HITBOX){.x=-1,.y=-1,.w=10,.h=10};
}
static void BALLOON_update(OBJ* this) {
	if (this->spr==22) {
		this->offset+=0.01;
		this->y=this->start+P8sin(this->offset)*2;
		OBJ* hit = OBJ_collide(this, OBJ_PLAYER, 0,0);
		if (hit != NULL && hit->djump<max_djump) {
			psfx(6);
			init_object(OBJ_SMOKE,this->x,this->y);
			hit->djump=max_djump;
			this->spr=0;
			this->timer=60;
		}
	} else if (this->timer>0) {
		this->timer-=1;
	} else { 
		psfx(7);
		init_object(OBJ_SMOKE,this->x,this->y);
		this->spr=22;
	}
}
static void BALLOON_draw(OBJ* this) {
	if (this->spr==22) {
		P8spr(13+(int)(this->offset*8)%3,this->x,this->y+6,   1,1,false,false);
		P8spr(this->spr,this->x,this->y,   1,1,false,false);
	}
}


//fall_floor
static void FALL_FLOOR_init(OBJ* this) {
	this->state=0;
	//this->solid=true; //this is a typo.. not fixing in order to maintain original behaviour
}
static void FALL_FLOOR_update(OBJ* this) {
	// idling
	if (this->state == 0) {
		if (OBJ_check(this, OBJ_PLAYER,0,-1) || OBJ_check(this, OBJ_PLAYER,-1,0) || OBJ_check(this, OBJ_PLAYER,1,0)) {
			break_fall_floor(this);
		}
	// shaking
	} else if (this->state==1) {
		this->delay-=1;
		if (this->delay<=0) {
			this->state=2;
			this->delay=60;//how long it hides for
			this->collideable=false;
		}
	// invisible, waiting to reset
	} else if (this->state==2) {
		this->delay-=1;
		if (this->delay<=0 && !OBJ_check(this, OBJ_PLAYER,0,0)) {
			psfx(7);
			this->state=0;
			this->collideable=true;
			init_object(OBJ_SMOKE,this->x,this->y);
		}
	}
}
static void FALL_FLOOR_draw(OBJ* this) {
	if (this->state!=2) {
		if (this->state!=1) {
			P8spr(23,this->x,this->y,   1,1,false,false);
		} else {
			P8spr(23+(15-this->delay)/5,this->x,this->y,   1,1,false,false);
		}
	}
}

static void break_fall_floor(OBJ* obj) {
	if (obj->state==0) {
		psfx(15);
		obj->state=1;
		obj->delay=15;//how long until it falls
		init_object(OBJ_SMOKE,obj->x,obj->y);
		OBJ* hit=OBJ_collide(obj, OBJ_SPRING,0,-1);
		if (hit != NULL) {
			break_spring(hit);
		}
	}
}

//smoke
static void SMOKE_init(OBJ* this) {
	this->spr=29;
	this->spd.y=-0.1;
	this->spd.x=0.3+P8rnd(0.2);
	this->x+=-1+P8rnd(2);
	this->y+=-1+P8rnd(2);
	this->flip_x=maybe();
	this->flip_y=maybe();
	this->solids=false;
}
static void SMOKE_update(OBJ* this) {
	this->spr+=0.2;
	if (this->spr>=32) {
		destroy_object(this);
	}
}

//fruit
	//tile=26,
	//if_not_fruit=true,
static void FRUIT_init(OBJ* this) { 
	this->start=this->y;
	this->off=0;
}
static void FRUIT_update(OBJ* this) {
	OBJ* hit=OBJ_collide(this, OBJ_PLAYER,0,0);
	if (hit!=NULL) {
		hit->djump=max_djump;
		sfx_timer=20;
		P8sfx(13);
		got_fruit[level_index()] = true;
		init_object(OBJ_LIFEUP,this->x,this->y);
		destroy_object(this);
	}
	this->off+=1;
	this->y=this->start+P8sin(this->off/40)*2.5;
}

//fly_fruit
	//tile=28,
	//if_not_fruit=true,
static void FLY_FRUIT_init(OBJ* this) { 
	this->start=this->y;
	this->fly=false;
	this->step=0.5;
	this->solids=false;
	this->sfx_delay=8;
}
static void FLY_FRUIT_update(OBJ* this) {
	int do_destroy_object = 0; //LEMON: see PLAYER_update..
	//fly away
	if (this->fly) {
		if (this->sfx_delay>0) {
			this->sfx_delay-=1;
			if (this->sfx_delay<=0) {
				sfx_timer=20;
				P8sfx(14);
			}
		}
		this->spd.y=appr(this->spd.y,-3.5,0.25);
		if (this->y<-16) {
			do_destroy_object = 1;
		}
	// wait
	} else {
		if (has_dashed) {
			this->fly=true;
		}
		this->step+=0.05;
		this->spd.y=P8sin(this->step)*0.5;
	}
	// collect
	OBJ* hit=OBJ_collide(this, OBJ_PLAYER,0,0);
	if (hit!=NULL) {
		hit->djump=max_djump;
		sfx_timer=20;
		P8sfx(13);
		got_fruit[level_index()] = true;
		init_object(OBJ_LIFEUP,this->x,this->y);
		do_destroy_object = 1;
	}
	if (do_destroy_object) destroy_object(this);
}
static void FLY_FRUIT_draw(OBJ* this) {
	float off=0;
	if (!this->fly) {
		float dir=P8sin(this->step);
		if (dir<0) {
			off=1+P8max(0,sign(this->y-this->start));
		}
	} else {
		off=P8modulo(off+0.25, 3);
	}
	P8spr(45+off,this->x-6,this->y-2,	1,1,true,false);
	P8spr(this->spr,this->x,this->y,	1,1,false,false);
	P8spr(45+off,this->x+6,this->y-2,	1,1,false,false);
}

//lifeup
static void LIFEUP_init(OBJ* this) {
	this->spd.y=-0.25;
	this->duration=30;
	this->x-=2;
	this->y-=4;
	this->flash=0;
	this->solids=false;
}
static void LIFEUP_update(OBJ* this) {
	this->duration-=1;
	if (this->duration<= 0) {
		destroy_object(this);
	}
}
static void LIFEUP_draw(OBJ* this) {
	this->flash+=0.5;

	P8print("1000",this->x-2,this->y,7+((int)this->flash)%2);
}

//fake_wall
	//tile=64,
	//if_not_fruit=true,
static void FAKE_WALL_update(OBJ* this) {
	this->hitbox=(HITBOX){.x=-1,.y=-1,.w=18,.h=18};
	OBJ* hit = OBJ_collide(this, OBJ_PLAYER,0,0);
	if (hit!=NULL && hit->dash_effect_time>0) {
		hit->spd.x=-sign(hit->spd.x)*1.5;
		hit->spd.y=-1.5;
		hit->dash_time=-1;
		sfx_timer=20;
		P8sfx(16);
		//destroy_object(this);
		init_object(OBJ_SMOKE,this->x,this->y);
		init_object(OBJ_SMOKE,this->x+8,this->y);
		init_object(OBJ_SMOKE,this->x,this->y+8);
		init_object(OBJ_SMOKE,this->x+8,this->y+8);
		init_object(OBJ_FRUIT,this->x+4,this->y+4);
		destroy_object(this); //LEMON: moved here. see PLAYER_update
	}
	this->hitbox=(HITBOX){.x=0,.y=0,.w=16,.h=16};
}
static void FAKE_WALL_draw(OBJ* this) {
	P8spr(64,this->x,this->y,	1,1,false,false);
	P8spr(65,this->x+8,this->y,	1,1,false,false);
	P8spr(80,this->x,this->y+8,	1,1,false,false);
	P8spr(81,this->x+8,this->y+8,	1,1,false,false);
}

//key
	//tile=8,
	//if_not_fruit=true,
static void KEY_update(OBJ* this) {
	int was=P8flr(this->spr);
	this->spr=9+(P8sin(frames/30.0)+0.5)*1;
	int is=P8flr(this->spr);
	if (is==10 && is!=was) {
		this->flip_x=!this->flip_x;
	}
	if (OBJ_check(this, OBJ_PLAYER,0,0)) {
		P8sfx(23);
		sfx_timer=10;
		destroy_object(this);
		has_key=true;
	}
}

//chest
	//tile=20,
	//if_not_fruit=true,
static void CHEST_init(OBJ* this) {
	this->x-=4;
	this->start=this->x;
	this->timer=20;
}
static void CHEST_update(OBJ* this) {
	if (has_key) {
		this->timer-=1;
		this->x=this->start-1+P8rnd(3);
		if (this->timer<=0) {
			sfx_timer=20;
			P8sfx(16);
			init_object(OBJ_FRUIT,this->x,this->y-4);
			destroy_object(this);
		}
	}
}

//platform
static void PLATFORM_init(OBJ* this) {
	this->x-=4;
	this->solids=false;
	this->hitbox.w=16;
	this->last=this->x;
}
static void PLATFORM_update(OBJ* this) {
	this->spd.x=this->dir*0.65;
	if (this->x<-16) { this->x=128;
	} else if (this->x>128) { this->x=-16; }
	if (!OBJ_check(this, OBJ_PLAYER,0,0)) {
		OBJ* hit=OBJ_collide(this, OBJ_PLAYER,0,-1);
		if (hit!=NULL) {
			OBJ_move_x(hit, this->x-this->last,1);
		}
	}
	this->last=this->x;
}
static void PLATFORM_draw(OBJ* this) {
	P8spr(11,this->x,this->y-1,	 1,1,false,false);
	P8spr(12,this->x+8,this->y-1,   1,1,false,false);
}

//message
	//tile=86,
	//last=0,
static void MESSAGE_draw(OBJ* this) {
	this->text="-- celeste mountain --#this memorial to those# perished on the climb";
	if (OBJ_check(this, OBJ_PLAYER,4,0)) {
		if (this->index<strlen(this->text)) {
			this->index+=0.5;
			if (this->index>=this->last+1) {
				this->last+=1;
				P8sfx(35);
			}
		}
		this->off2.x=8;
		this->off2.y=96;
		for (int i=0; i<this->index; i++) {
			if (this->text[i]!='#') {
				P8rectfill(this->off2.x-2,this->off2.y-2,this->off2.x+7,this->off2.y+6, 7);
				char charstr[2];
				charstr[0] = this->text[i], charstr[1] = '\0';
				P8print(charstr,this->off2.x,this->off2.y,0);
				this->off2.x+=5;
			} else {
				this->off2.x=8;
				this->off2.y+=7;
			}
		}
	} else {
		this->index=0;
		this->last=0;
	}
}

//big_chest
	//tile=96,
static void BIG_CHEST_init(OBJ* this) {
	this->state=0;
	this->hitbox.w=16;
}
static void BIG_CHEST_draw(OBJ* this) {
	if (this->state==0) {
		OBJ* hit=OBJ_collide(this, OBJ_PLAYER,0,8);
		if (hit!=NULL && OBJ_is_solid(hit, 0,1)) {
			P8music(-1,500,7);
			P8sfx(37);
			pause_player=true;
			hit->spd.x=0;
			hit->spd.y=0;
			this->state=1;
			init_object(OBJ_SMOKE,this->x,this->y);
			init_object(OBJ_SMOKE,this->x+8,this->y);
			this->timer=60;
			this->particle_count = 0;
		}
		P8spr(96,this->x,this->y,   1,1,false,false);
		P8spr(97,this->x+8,this->y,  1,1,false,false);
	} else if (this->state==1) {
		this->timer-=1;
		shake=5;
		flash_bg=true;
		if (this->timer<=45 && this->particle_count<50) {
			this->particles[this->particle_count++] = (PARTICLE){
				.x=1+P8rnd(14),
				.y=0,
				.h=32+P8rnd(32),
				.spd=8+P8rnd(8)
			};
		}
		if (this->timer<0) {
			this->state=2;
			this->particle_count=0;
			flash_bg=false;
			new_bg=true;
			init_object(OBJ_ORB,this->x+4,this->y+4);
			pause_player=false;
		}
		for (int i = 0; i < this->particle_count; i++) {
			PARTICLE* p = &this->particles[i];
			p->y+=p->spd;
			P8line(this->x+p->x,this->y+8-p->y,this->x+p->x,P8min(this->y+8-p->y+p->h,this->y+8),7);
		}
	}
	P8spr(112,this->x,this->y+8,   1,1,false,false);
	P8spr(113,this->x+8,this->y+8, 1,1,false,false);
}

//orb
static void ORB_init(OBJ* this) {
	this->spd.y=-4;
	this->solids=false;
	this->particle_count = 0;
}
static void ORB_draw(OBJ* this) {
	this->spd.y=appr(this->spd.y,0,0.5);
	OBJ* hit=OBJ_collide(this, OBJ_PLAYER,0,0);
	if (this->spd.y==0 && hit!=NULL) {
		music_timer=45;
		P8sfx(51);
		freeze=10;
		shake=10;
		destroy_object(this);
		max_djump=2;
		hit->djump=2;
	}
   
	P8spr(102,this->x,this->y,  1,1,false,false);
	float off=frames/30.0;
	for (int i=0; i <= 7; i++) {
		P8circfill(this->x+4+P8cos(off+i/8.0)*8,this->y+4+P8sin(off+i/8.0)*8,1,7);
	}
}

//flag
	//tile=118,
static void FLAG_init(OBJ* this) {
	this->x+=5;
	this->score=0;
	this->show=false;
	for (int i=0; i < FRUIT_COUNT; i++) {
		if (got_fruit[i]) {
			this->score+=1;
		}
	}
}
static void FLAG_draw(OBJ* this) {
	this->spr=118+(frames/5)%3;
	P8spr(this->spr,this->x,this->y, 1,1,false,false);
	if (this->show) {
		P8rectfill(32,2,96,31,0);
		P8spr(26,55,6, 1,1,false,false);
		{
			char str[16];
			snprintf(str, sizeof(str), "x%i", this->score);
			P8print(str,64,9,7);
		}
		draw_time(49,16);
		{
			char str[16];
			snprintf(str, sizeof(str), "deaths:%i", deaths);
			P8print(str,48,24,7);
		}
	} else if (OBJ_check(this, OBJ_PLAYER,0,0)) {
		P8sfx(55);
		sfx_timer=30;
		this->show=true;
	}
}

//room_title
static void ROOM_TITLE_init(OBJ* this) {
	this->delay=5;
}
static void ROOM_TITLE_draw(OBJ* this) {
	this->delay-=1;
	if (this->delay<-30) {
		destroy_object(this);
	} else if (this->delay<0) {
		P8rectfill(24,58,104,70,0);
		//rect(26,64-10,102,64+10,7)
		//print("//-",31,64-2,13)
		if (room.x==3 && room.y==1) {
			P8print("old site",48,62,7);
		} else if (level_index()==30) {
			P8print("summit",52,62,7);
		} else {
			int level=(1+level_index())*100;
			{
				char str[16];
				snprintf(str, sizeof(str), "%i m", level);
				P8print(str,52+(level<1000 ? 2 : 0),62,7);
			}
		}
		//print("//-",86,64-2,13)
		 
		draw_time(4,4);
	}
}

// object functions //
//////////////////////-

static OBJ* init_object(OBJTYPE type, float x, float y) {
	//if (type.if_not_fruit!=NULL && got_fruit[1+level_index()]) {
	if ((type == OBJ_FRUIT || type == OBJ_FLY_FRUIT) && got_fruit[1+level_index()]) {
		return NULL;
	}
	OBJ* obj = NULL;
	for (int i = 0; i < MAX_OBJECTS; i++) {
		if (!objects[i].active) {
			obj = &objects[i];
			break;
		}
	}
	if (!obj) {
		//no more free space for objects, give up
		// printf("exhausted object memory..\n");
		return NULL;
	}
	obj->active = true;

	obj->type = type;
	obj->collideable=true;
	obj->solids=true;

	obj->spr = OBJTYPE_prop(type).tile;
	obj->flip_x = obj->flip_y = false;

	obj->x = x;
	obj->y = y;
	obj->hitbox = (HITBOX){ .x=0,.y=0,.w=8,.h=8 };

	obj->spd = (VEC){.x=0,.y=0};
	obj->rem = (VEC){.x=0,.y=0};

	//add(objects,obj)
	if (OBJTYPE_prop(obj->type).init!=NULL) {
		OBJTYPE_prop(obj->type).init(obj);
	}
	return obj;
}

static void destroy_object(OBJ* obj) {
	obj->active = false;
}

static void kill_player(OBJ* obj) {
	sfx_timer=12;
	P8sfx(0);
	deaths+=1;
	shake=10;
	destroy_object(obj);
	dead_particles_count = 0;
	for (float dir=0; dir <= 7; dir++) {
		float angle=(dir/8);
		dead_particles[dead_particles_count++] = (PARTICLE){
			.active = true,
			.x=obj->x+4,
			.y=obj->y+4,
			.t=10,
			.spd2=(VEC){
				.x=P8sin(angle)*3,
				.y=P8cos(angle)*3
			}
		};
		restart_room();
	}
}

// room functions //
////////////////////

static void restart_room() {
	will_restart=true;
	delay_restart=15;
}

static void next_room() {
	if (room.x==2 && room.y==1) {
		P8music(30,500,7);
	} else if (room.x==3 && room.y==1) {
		P8music(20,500,7);
	} else if (room.x==4 && room.y==2) {
		P8music(30,500,7);
	} else if (room.x==5 && room.y==3) {
		P8music(30,500,7);
	}

	if (room.x==7) {
		load_room(0,room.y+1);
	} else {
		load_room(room.x+1,room.y);
	}
}

static void load_room(int x, int y) {
	has_dashed=false;
	has_key=false;

	//remove existing objects
	for (int i = 0; i < MAX_OBJECTS; i++) {
		objects[i].active = false;
	}

	//current room
	room.x = x;
	room.y = y;

	// entities
	for (int tx=0; tx <= 15; tx++) {
		for (int ty=0; ty <= 15; ty++) {
			int tile = P8mget(room.x*16+tx,room.y*16+ty);
			if (tile==11) {
				init_object(OBJ_PLATFORM,tx*8,ty*8)->dir=-1;
			} else if (tile==12) {
				init_object(OBJ_PLATFORM,tx*8,ty*8)->dir=1;
			} else {
				for (int type = 0; type < OBJTYPE_COUNT; type++) {
					if (tile == OBJTYPE_prop(type).tile) {
						init_object(type, tx*8, ty*8);
					}
				}
			}
		}
	}
   
	if (!is_title()) {
		init_object(OBJ_ROOM_TITLE,0,0);
	}
}

// update function //
//////////////////////-

void Celeste_P8_update() {
	frames=((frames+1)%30);
	if (frames==0 && level_index()<30) {
		seconds=((seconds+1)%60);
		if (seconds==0) {
			minutes+=1;
		}
	}
   
	if (music_timer>0) {
		music_timer-=1;
		if (music_timer<=0) {
			P8music(10,0,7);
		}
	}
   
	if (sfx_timer>0) {
		sfx_timer-=1;
	}
   
	// cancel if (freeze
	if (freeze>0) { freeze-=1; return; }

	// screenshake
	if (shake>0) {
		shake-=1;
		P8camera(0,0);
		if (shake>0) {
			P8camera(-2+P8rnd(5),-2+P8rnd(5));
		}
	}
   
	// restart (soon)
	if (will_restart && delay_restart>0) {
		delay_restart-=1;
		if (delay_restart<=0) {
			will_restart=false;
			load_room(room.x,room.y);
		}
	}

	// update each object
	for (int i = 0; i < MAX_OBJECTS; i++) {
		OBJ* obj = &objects[i];
		if (!obj->active) continue;

		OBJ_move(obj, obj->spd.x,obj->spd.y);
		if (OBJTYPE_prop(obj->type).update!=NULL) {
			OBJTYPE_prop(obj->type).update(obj);
		}
	}
   
	// start game
	if (is_title()) {
		if (!start_game && (P8btn(k_jump) || P8btn(k_dash))) {
			P8music(-1, 0, 0);
			start_game_flash=50;
			start_game=true;
			P8sfx(38);
		}
		if (start_game) {
			start_game_flash-=1;
			if (start_game_flash<=-30) {
				begin_game();
			}
		}
	}
}

// drawing functions //
//////////////////////-
void Celeste_P8_draw() {
	if (freeze>0) { return; }
   
	// reset all palette values
	P8pal_reset();
   
	// start game flash
	if (start_game) {
		int c=10;
		if (start_game_flash>10) {
			if (frames%10<5) {
				c=7;
			}
		} else if (start_game_flash>5) {
			c=2;
		} else if (start_game_flash>0) {
			c=1;
		} else { 
			c=0;
		}
		if (c<10) {
			P8pal(6,c),
			P8pal(12,c);
			P8pal(13,c);
			P8pal(5,c);
			P8pal(1,c);
			P8pal(7,c);
		}
	}

	// clear screen
	int bg_col = 0;
	if (flash_bg) {
		bg_col = frames/5.0;
	} else if (new_bg) {
		bg_col=2;
	}
	P8rectfill(0,0,128,128,bg_col);

	// clouds
	if (!is_title()) {
		CLOUD* c = &clouds[0];
		while (!c->isLast) {
			c->x += c->spd;
			P8rectfill(c->x,c->y,c->x+c->w,c->y+4+(1-c->w/64.0)*12,new_bg ? 14 : 1);
			if (c->x > 128) {
				c->x = -c->w;
				c->y = P8rnd(128-8);
			}
			c++;
		}
	}

	// draw bg terrain
	P8map(room.x * 16,room.y * 16,0,0,16,16,4);

	// platforms/big chest
	for (int i = 0; i < MAX_OBJECTS; i++) {
		OBJ* o = &objects[i];
		if (o->active && (o->type==OBJ_PLATFORM || o->type==OBJ_BIG_CHEST)) {
			draw_object(o);
		}
	}

	// draw terrain
	int off=is_title() ? -4 : 0;
	P8map(room.x*16,room.y * 16,off,0,16,16,2);
   
	// draw objects
	for (int i = 0; i < MAX_OBJECTS; i++) {
		OBJ* o = &objects[i];
		if (o->active && (o->type!=OBJ_PLATFORM && o->type!=OBJ_BIG_CHEST)) {
			draw_object(o);
		}
	}
   
	// draw fg terrain
	P8map(room.x * 16,room.y * 16,0,0,16,16,8);
   
	// particles
	PARTICLE* p = &particles[0];
	while (!p->isLast) {
		p->x += p->spd;
		p->y += P8sin(p->off);
		p->off+= P8min(0.05,p->spd/32);
		P8rectfill(p->x,p->y,p->x+p->s,p->y+p->s,p->c);
		if (p->x>128+4) { 
			p->x=-4;
			p->y=P8rnd(128);
		}
		p++;
	}
   
	// dead particles
	p = &dead_particles[0];
	while (!p->isLast) {
		if (p->active) {
			p->x += p->spd2.x;
			p->y += p->spd2.y;
			p->t -=1;
			if (p->t <= 0) { p->active = false; }
			P8rectfill(p->x-p->t/5,p->y-p->t/5,p->x+p->t/5,p->y+p->t/5,14+P8modulo(p->t,2));
		}

		p++;
	}
   
	// draw outside of the screen for screenshake
	P8rectfill(-5,-5,-1,133,0);
	P8rectfill(-5,-5,133,-1,0);
	P8rectfill(-5,128,133,133,0);
	P8rectfill(128,-5,133,133,0);
   
	// credits
	if (is_title()) {
		P8print("x+c",58,80,5);
		P8print("matt thorson",42,96,5);
		P8print("noel berry",46,102,5);
	}
   
	if (level_index()==30) {
		OBJ* p = NULL;
		for (int i = 0; i < MAX_OBJECTS; i++) {
			if (objects[i].active && objects[i].type==OBJ_PLAYER) {
				p = &objects[i];
				break;
			}
		}
		if (p!=NULL) {
			float diff=P8min(24,40-P8abs(p->x+4-64));
			P8rectfill(0,0,diff,128,0);
			P8rectfill(128-diff,0,128,128,0);
		}
	}
}

static void draw_object(OBJ* obj) {
	if (OBJTYPE_prop(obj->type).draw !=NULL) {
		OBJTYPE_prop(obj->type).draw(obj);
	} else if (obj->spr > 0) {
		P8spr(obj->spr,obj->x,obj->y,1,1,obj->flip_x,obj->flip_y);
	}

}

static void draw_time(float x, float y) {
	int s=seconds;
	int m=minutes%60;
	int h=minutes/60;
   
	P8rectfill(x,y,x+32,y+6,0);
	{
		char str[27];
		snprintf(str, sizeof(str), "%.2i:%.2i:%.2i", h, m, s);
		P8print(str,x+1,y+1,7);
	}
}

// helper functions //
//////////////////////
static float clamp(float val, float a, float b) {

	return P8max(a, P8min(b, val));
}
static float appr(float val, float target, float amount) {
	return val > target 
		? P8max(val - amount, target) 
		: P8min(val + amount, target);
}

static int sign(float v) {
	return v > 0 ? 1 : (v < 0 ? -1 : 0);
}
static bool maybe() {

	return P8rnd(1)<0.5;
}

static bool solid_at(int x,int y,int w,int h) {
	return tile_flag_at(x,y,w,h,0);
}

static bool ice_at(int x,int y,int w,int h) {
	return tile_flag_at(x,y,w,h,4);
}

static bool tile_flag_at(int x,int y,int w,int h,int flag) {
	for (int i=P8max(0,P8flr(x/8)); i <= P8min(15,(x+w-1)/8); i++) {
		 for (int j=P8max(0,P8flr(y/8)); j <= P8min(15,(y+h-1)/8); j++) {
			if (P8fget(tile_at(i,j),flag)) {
				return true;
			}
		}
	}
	return false;
}

static int tile_at(int x,int y) {
	return P8mget(room.x * 16 + x, room.y * 16 + y);
}

static bool spikes_at(float x,float y,int w,int h,float xspd,float yspd) {
	for (int i=P8max(0,P8flr(x/8)); i <= P8min(15,(x+w-1)/8); i++) {
		for (int j=P8max(0,P8flr(y/8)); j <= P8min(15,(y+h-1)/8); j++) {
			int tile=tile_at(i,j);
			if (tile==17 && (P8modulo(y+h-1, 8)>=6 || y+h==j*8+8) && yspd>=0) {
				return true;
			} else if (tile==27 && P8modulo(y, 8)<=2 && yspd<=0) {
				return true;
			} else if (tile==43 && P8modulo(x, 8)<=2 && xspd<=0) {
				return true;
			} else if (tile==59 && (P8modulo(x+w-1, 8)>=6 || x+w==i*8+8) && xspd>=0) {
				return true;
			}
		}
	}
	return false;
}

//////////END/////////

void Celeste_P8__DEBUG(void) {
	if (is_title()) start_game = true, start_game_flash=1;
	else next_room();
}

//all of the global game variables; this holds the entire game state (exc. music/sounds playing)
#define LISTGVARS(V) \
	V(room) V(freeze) V(shake) V(will_restart) V(delay_restart) V(got_fruit) \
	V(has_dashed) V(sfx_timer) V(has_key) V(pause_player) V(flash_bg) V(music_timer) \
	V(new_bg) V(frames) V(seconds) V(minutes) V(deaths) V(max_djump) V(start_game) \
	V(start_game_flash) V(k_left) V(k_right) V(k_up) V(k_down) V(k_dash) V(clouds) \
	V(particles) V(particle_count) V(dead_particles) V(dead_particles_count) V(objects)

size_t Celeste_P8_get_state_size(void) {
#define V_SIZE(v) (sizeof v) +
	enum { //force comptime evaluation
		sz = LISTGVARS(V_SIZE) - 0
	};
	return sz;
#undef V_SIZE
};

#include <assert.h>

void Celeste_P8_save_state(void* st) {
	assert(st != NULL);
#define V_SAVE(v) memcpy(st, &v, sizeof v), st += sizeof v;
	LISTGVARS(V_SAVE)
#undef V_SAVE
}
void Celeste_P8_load_state(void* st) {
	assert(st != NULL);
#define V_LOAD(v) memcpy(&v, st, sizeof v), st += sizeof v;
	LISTGVARS(V_LOAD)
#undef V_LOAD
}

#undef LISTGVARS
