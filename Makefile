CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -g
TARGET=lwserver
SOURCES=main.c socket.c handler.c parser.c utils.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

.PHONY: all clean