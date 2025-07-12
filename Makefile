CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -g
TARGET=lwserver
SOURCES=main.c socket.c handler.c parser.c utils.c
OBJDIR=build
OBJS=$(patsubst %.c,$(OBJDIR)/%.o,$(SOURCES))

all: $(OBJDIR) $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(OBJDIR)/$(TARGET) $(OBJS)

clean:
	rm -rf $(OBJDIR)

.PHONY: all clean