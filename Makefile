CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS = -lssl -lcrypto -lzstd 

TARGET = lwserver
SOURCES = main.c socket.c handler.c parser.c utils.c html_handler.c hot_reload.c tsl-ssl.c globals.c
OBJDIR = build
OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(SOURCES))

all: $(OBJDIR) $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(OBJDIR)/$(TARGET) $(OBJS) $(LDFLAGS)

clean:
	rm -rf $(OBJDIR)

.PHONY: all clean
