LDFLAGS = -lasound -lm -lpigpio
CFLAGS = -Wall -std=gnu99 # gnu99 so that popen is defined

all: main.c
	gcc main.c $(LDFLAGS) $(CFLAGS) -o final
