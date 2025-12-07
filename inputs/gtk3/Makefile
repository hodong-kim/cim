include ../../config.mk

GTK3_BINARY_VERSION=`pkg-config --variable=gtk_binary_version gtk+-3.0`

IM_CIM_GTK3_DEPS_CFLAGS = \
	`pkg-config --cflags gtk+-3.0` \
	-I$(TOP_SRCDIR)/libcim

IM_CIM_GTK3_DEPS_LIBS = \
	`pkg-config --libs   gtk+-3.0` \
	$(TOP_SRCDIR)/libcim/libcim.a

CFLAGS = \
	$(EXTRA_CFLAGS) \
	-I$(TOP_SRCDIR)/libcim \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
	-DCIM_LOCALE_DIR=\"$(CIM_LOCALE_DIR)\"

SOURCES = im-cim-gtk.c

# for gtk3
gtk3_im_moduledir = $(GTK3_LIBDIR)/gtk-3.0/$(GTK3_BINARY_VERSION)/immodules
IM_CIM_GTK3 = im-cim-gtk3.so

im_cim_gtk3_CFLAGS  = $(IM_CIM_GTK3_DEPS_CFLAGS) $(CFLAGS)
im_cim_gtk3_LDFLAGS = -Wl,--as-needed $(IM_CIM_GTK3_DEPS_LIBS)

all: $(IM_CIM_GTK3)

$(IM_CIM_GTK3): im-cim-gtk3.o c-candidate.o Makefile
	$(CC) -shared -fPIC $(EXTRA_LDFLAGS) im-cim-gtk3.o c-candidate.o $(im_cim_gtk3_LDFLAGS) \
	  -Wl,-soname -Wl,$(IM_CIM_GTK3) \
	  -o $(IM_CIM_GTK3)

im-cim-gtk3.o: $(SOURCES) Makefile $(TOP_SRCDIR)/libcim/libcim.a
	$(CC) -fPIC $(im_cim_gtk3_CFLAGS) -c $(SOURCES) -o im-cim-gtk3.o

c-candidate.o: c-candidate.c Makefile
	$(CC) -fPIC $(im_cim_gtk3_CFLAGS) -c c-candidate.c -o c-candidate.o

install:
	mkdir -p $(DESTDIR)$(gtk3_im_moduledir)
	install -m 755 $(IM_CIM_GTK3) $(DESTDIR)$(gtk3_im_moduledir)

uninstall:
	rm -f $(DESTDIR)$(gtk3_im_moduledir)/$(IM_CIM_GTK3)
	-rmdir -p $(DESTDIR)$(gtk3_im_moduledir)

clean:
	rm -f $(IM_CIM_GTK3) im-cim-gtk3.o c-candidate.o
