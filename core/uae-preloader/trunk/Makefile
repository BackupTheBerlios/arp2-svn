
uae-preloader: preloader.o Makefile
	$(CC) -o $@ -static -nostartfiles -nodefaultlibs -Wl,-Ttext=0xb8000000 preloader.o $(LIBPORT) $(LDFLAGS)

clean:
	$(RM) preloader.o uae-preloader
