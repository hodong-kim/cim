include ../config.mk

GTK3_DEPS_CFLAGS = `pkg-config --cflags gtk+-3.0`
GTK3_DEPS_LIBS   = `pkg-config --libs   gtk+-3.0`

TEST_CIM_CFLAGS = -I$(TOP_SRCDIR)/libcim
TEST_CIM_LIBS   = $(TOP_SRCDIR)/libcim/libcim.a

SOURCE = gtk3.c

all: gtk3 test-cim

gtk3: Makefile $(SOURCE)
	$(CC) $(GTK3_DEPS_CFLAGS) $(SOURCE) $(GTK3_DEPS_LIBS) -o gtk3

test-cim: Makefile test-cim.c
	$(CC) $(TEST_CIM_CFLAGS) test-cim.c $(TEST_CIM_LIBS) -o test-cim

install:

clean:
	rm -f gtk3 *.core vgcore.* test-cim
