MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables

CC = gcc

BINARY = embedded_invaders

CFLAGS = -std=gnu11
CFLAGS += -O0 -g
CFLAGS += -m32
CFLAGS += -Wall -Wextra -Werror=implicit
CFLAGS += -Wmissing-prototypes -Wstrict-prototypes -Wshadow

SOURCES = \
	src/main.c \
	src/eetg.c \
	src/ei.c

OBJECTS = $(patsubst %.S,%.o,$(patsubst %.c,%.o,$(SOURCES)))

$(BINARY): $(OBJECTS)
	$(CC) -o $@ $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(BINARY) $(OBJECTS)

.PHONY: clean $(SOURCES)
