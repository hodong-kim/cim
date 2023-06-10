include ../config.mk

all: libcim.a

LIBCIM_MAJOR = 0
LIBCIM_MINOR = 0
LIBCIM_MICRO = 0
LIBCIM_VERSION = $(LIBCIM_MAJOR).$(LIBCIM_MINOR).$(LIBCIM_MICRO)

CLAIR_C_SOURCES = \
	c-array.c \
	c-log.c \
	c-mem.c \
	c-str.c \
	c-utils.c

CLAIR_H_SOURCES = \
	c-array.h \
	c-log.h \
	c-macros.h \
	c-mem.h \
	c-progname.h \
	c-str.h \
	c-types.h \
	c-utils.h

C_SOURCES = cim.c $(CLAIR_C_SOURCES)
H_SOURCES = cim.h $(CLAIR_H_SOURCES)

SOURCES   = $(H_SOURCES) $(C_SOURCES)

CFLAGS    = -I. $(EXTRA_CFLAGS)
DEPS_LIBS =
LDFLAGS   = $(EXTRA_LDFLAGS) -Wl,--as-needed $(DEPS_LIBS)

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
	rm -f $(C_SOURCES:.c=.o) libcim.a *.core vgcore.*
	rm -f libcim.so \
				libcim.so.$(LIBCIM_VERSION) \
				libcim.so.$(LIBCIM_MAJOR).$(LIBCIM_MINOR) \
				libcim.so.$(LIBCIM_MAJOR)
