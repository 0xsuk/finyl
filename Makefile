LDFLAGS = -lasound -lm
CFLAGS = -Wall

all: main.c
	gcc main.c $(LDFLAGS) $(CFLAGS) -o final
