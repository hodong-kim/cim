GTK3_DEPS_CFLAGS = `pkg-config --cflags gtk+-3.0`
GTK3_DEPS_LIBS   = `pkg-config --libs   gtk+-3.0`

SOURCE = gtk3.c

all: gtk3

gtk3: Makefile $(SOURCE)
	$(CC) $(GTK3_DEPS_CFLAGS) $(SOURCE) $(GTK3_DEPS_LIBS) -o gtk3

install:

clean:
	rm -f gtk3 *.core vgcore.*
