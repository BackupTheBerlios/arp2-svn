
CFLAGS		= -Os

all:	uae-preloader libuae-preloader.so

uae-preloader: preloader.o Makefile
	$(CC) -o $@ -static -nostartfiles -nodefaultlibs -Wl,-Ttext=0xb8000000 preloader.o $(LIBPORT) $(LDFLAGS)

libuae-preloader.so:	libuae-preloader.c
	$(CC) $(CFLAGS) -o $@ -shared $^

clean:
	$(RM) preloader.o uae-preloader libuae-preloader.o libuae-preloader.so
