include ../../config.mk

DEPS_CFLAGS = `pkg-config --cflags Qt6Core Qt6Gui Qt6Widgets gobject-2.0 gio-2.0` \
	-I$(TOP_SRCDIR)/libcim

DEPS_LIBS   = `pkg-config --libs   Qt6Core Qt6Gui Qt6Widgets gobject-2.0 gio-2.0` \
	$(TOP_SRCDIR)/libcim/libcim.a

TARGET = libqt6im-cim.so

BUILT_SOURCES = im-cim-qt.moc
SOURCES       = im-cim-qt.cpp $(BUILT_SOURCES)

CXXFLAGS = \
	$(EXTRA_CXXFLAGS) \
	-I$(TOP_SRCDIR)/libnimf \
	-I$(QT6_CORE_PRIVATE_INCLUDE_PATH) \
	-I$(QT6_GUI_PRIVATE_INCLUDE_PATH) \
	$(DEPS_CFLAGS) \
	-DQT_NO_KEYWORDS \
	-std=c++17 \
	-DLIBNIMF_MAJOR=\"$(LIBNIMF_MAJOR)\"

LDFLAGS = -Wl,--as-needed $(DEPS_LIBS)

all: $(TARGET) $(BUILT_SOURCES)

$(TARGET): im-cim-qt6.o
	$(CXX) -shared -fPIC $(EXTRA_LDFLAGS) im-cim-qt6.o $(LDFLAGS) \
	  -Wl,-soname -Wl,$(TARGET) \
	  -o $(TARGET)

im-cim-qt6.o: $(SOURCES) Makefile
	$(CXX) -fPIC $(CXXFLAGS) -c im-cim-qt.cpp -o $@

im-cim-qt.moc: im-cim-qt.cpp
	$(MOC_QT6) -I $(QT6_CORE_PRIVATE_INCLUDE_PATH) \
	           -I $(QT6_GUI_PRIVATE_INCLUDE_PATH) im-cim-qt.cpp \
	           -o im-cim-qt.moc

install:
	mkdir -p $(DESTDIR)$(QT6_IM_MODULE_DIR)
	install -m 755 $(TARGET) $(DESTDIR)$(QT6_IM_MODULE_DIR)

uninstall:
	 rm    -f $(DESTDIR)$(QT6_IM_MODULE_DIR)/$(TARGET)
	-rmdir -p $(DESTDIR)$(QT6_IM_MODULE_DIR)

clean:
	rm -f $(TARGET) im-cim-qt6.o im-cim-qt.moc
