CC = gcc 
CFLAGS_SERVER = -Wall -Wextra  -pedantic -std=gnu99 -g  -I/local/courses/csse2310/include -L/local/courses/csse2310/lib -lcsse2310a4 -lfreeimage -lcsse2310_freeimage -pthread
CFLAGS_CLIENT = -Wall -Wextra  -pedantic -std=gnu99 -g  -I/local/courses/csse2310/include -L/local/courses/csse2310/lib -lcsse2310a4
all: uqimageclient uqimageproc

uqimageclient: uqimageclient.c
	$(CC) $(CFLAGS_CLIENT) -o $@ $<

uqimageproc: uqimageproc.c
	$(CC) $(CFLAGS_SERVER) -o $@ $<

clean: 
	rm uqimageclient uqimageproc
