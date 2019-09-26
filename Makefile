OBJS = $(patsubst %.c,%.o,$(shell find -name "*.c"))

LIBS = -lallegro -lallegro_image -lallegro_primitives -lallegro_font -lm
OUT = ./ccleste
CFLAGS = -Wall -Wno-unused-variable
PREFIX = /usr/bin

# link
$(OUT): $(OBJS) Makefile
	@echo "[INFO] linking"
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(OUT)

install: $(OUT)
	install $(OUT) $(PREFIX)

debug: CFLAGS += -ggdb
debug: $(OUT)

release: CFLAGS += -DNDEBUG
release: $(OUT)

#deprendencies
-include $(OBJS:.o=.d)

# generate dependendeces and  objetcs
%.o: %.c
	@echo "[INFO] generating deps for $*.o"
	$(CC) -MM $(CFLAGS) $*.c > $*.d
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	@echo

#  clean
clean:
	$(RM) $(OBJS) $%.d