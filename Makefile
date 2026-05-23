CC      = gcc
CFLAGS  = -Wall -Wextra -Iinclude
LDFLAGS = -lSDL2 -lm
TARGET  = visor
SRC     = src/main.c src/loader.c src/renderer.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: clean
