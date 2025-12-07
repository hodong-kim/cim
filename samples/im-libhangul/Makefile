include ../../config.mk

CIM_PLUGIN_DIR = $(libdir)/input.d

DEPS_CFLAGS = `pkg-config --cflags libhangul` -I$(TOP_SRCDIR)/libcim \
	$(INTL_CFLAG)

DEPS_LIBS   = `pkg-config --libs libhangul` \
	$(TOP_SRCDIR)/libcim/libcim.a \
	$(INTL_LDFLAG)

SOURCES = im-libhangul.c
CFLAGS  = \
	$(EXTRA_CFLAGS) \
	-I$(TOP_SRCDIR)/libcim \
	$(DEPS_CFLAGS)

LDFLAGS = $(EXTRA_LDFLAGS) -Wl,--as-needed $(DEPS_LIBS)

all: im-libhangul.so

im-libhangul.so: $(SOURCES:.c=.o) Makefile
	$(CC) -shared -fPIC $(SOURCES:.c=.o) $(LDFLAGS) \
	  -Wl,-soname -Wl,im-libhangul.so \
	  -o im-libhangul.so

.SUFFIXES: .c .o
.c.o:
	$(CC) -fPIC $(CFLAGS) -c $< -o $@

install:
	mkdir -p $(DESTDIR)$(CIM_PLUGIN_DIR)
	install -m 755 im-libhangul.so $(DESTDIR)$(CIM_PLUGIN_DIR)

uninstall:
	 rm    -f $(DESTDIR)$(CIM_PLUGIN_DIR)/im-libhangul.so
	-rmdir -p $(DESTDIR)$(CIM_PLUGIN_DIR)

clean:
	rm -f $(SOURCES:.c=.o) im-libhangul.so
