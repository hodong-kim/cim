include ../config.mk

LIBCIM_MAJOR = 0
LIBCIM_MINOR = 0
LIBCIM_MICRO = 0
LIBCIM_VERSION = $(LIBCIM_MAJOR).$(LIBCIM_MINOR).$(LIBCIM_MICRO)

C_SOURCES = \
	c-array.c \
	c-log.c \
	c-mem.c \
	c-str.c \
	c-utils.c \
	cim.c

H_SOURCES = \
	c-array.h \
	c-macros.h \
	c-progname.h \
	c-types.h \
	cim.h \
	c-log.h \
	c-mem.h \
	c-str.h \
	c-utils.h

SOURCES = $(H_SOURCES) $(C_SOURCES)

CFLAGS = \
	$(EXTRA_CFLAGS)

LDFLAGS = $(EXTRA_LDFLAGS)

all: libcim.a libcim.so

libcim.a: Makefile $(SOURCES) $(C_SOURCES:.c=.o)
	$(AR) rcs libcim.a $(C_SOURCES:.c=.o)

libcim.so: Makefile $(SOURCES) $(C_SOURCES:.c=.o)
	$(CC) $(CFLAGS) -shared -fPIC $(C_SOURCES:.c=.o) $(LDFLAGS) \
		-Wl,-soname -Wl,libcim.so.$(LIBCIM_MAJOR) \
		-o libcim.so.$(LIBCIM_VERSION)
	ln -sf libcim.so.$(LIBCIM_VERSION) libcim.so
	ln -sf libcim.so.$(LIBCIM_VERSION) libcim.so.$(LIBCIM_MAJOR)
	ln -sf libcim.so.$(LIBCIM_VERSION) libcim.so.$(LIBCIM_MAJOR).$(LIBCIM_MINOR)

.SUFFIXES: .c .o
.c.o: $(SOURCES)
	$(CC) -fPIC $(CFLAGS) -c $< -o $@

install:

uninstall:

clean:
	rm -f \
		$(C_SOURCES:.c=.o) \
		libcim.a \
		libcim.so \
		libcim.so.$(LIBCIM_VERSION) \
		libcim.so.$(LIBCIM_MAJOR).$(LIBCIM_MINOR) \
		libcim.so.$(LIBCIM_MAJOR) \
		*.core \
		vgcore.*
