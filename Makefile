CFLAGS=-Wall -g -O2
LDFLAGS=`sdl-config --cflags --libs` -lSDL_mixer
CELESTE_CC=$(CC)

ifneq ($(USE_FIXEDP),)
	OUT=ccleste-fixedp
	CELESTE_OBJ=celeste-fixedp.o
	CFLAGS+=-DCELESTE_P8_FIXEDP
	CELESTE_CC=$(CXX)
else
	OUT=ccleste
	CELESTE_OBJ=celeste.o
	LDFLAGS+=-lm
endif

all: $(OUT)

$(OUT): sdl12main.c $(CELESTE_OBJ) celeste.h
	$(CC) $(CFLAGS) $(LDFLAGS) sdl12main.c $(CELESTE_OBJ) -o $(OUT) 

$(CELESTE_OBJ): celeste.c celeste.h
	$(CELESTE_CC) $(CFLAGS) -c -o $(CELESTE_OBJ) celeste.c

clean:
	$(RM) ccleste ccleste-fixedp celeste.o
	make -f Makefile.3ds clean
