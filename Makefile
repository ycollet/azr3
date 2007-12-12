PACKAGE_NAME = azr3-jack
PACKAGE_VERSION = 0.1.668
PKG_DEPS = gtkmm-2.4>=2.8.8 jack>=0.107.0 lash-1.0>=0.5.3


PROGRAMS = azr3

azr3_SOURCES = \
	main.cpp main.hpp \
	azr3.cpp azr3.hpp \
	globals.hpp \
	filters.hpp \
	fx.hpp fx.cpp \
	voice_classes.hpp voice_classes.cpp \
	azr3gui.cpp azr3gui.hpp \
	knob.hpp knob.cpp \
	switch.hpp switch.cpp \
	drawbar.hpp drawbar.cpp \
	textbox.hpp textbox.cpp \
	cknob.xpm minioffon.xpm onoffgreen.xpm panelfx.xpm vonoff.xpm voice.xpm num_yellow.xpm dbblack.xpm dbbrown.xpm dbwhite.xpm
azr3_SOURCEDIR = azr3
azr3_CFLAGS = `pkg-config --cflags gtkmm-2.4 jack lash-1.0`
azr3_LDFLAGS = `pkg-config --libs gtkmm-2.4 jack lash-1.0`
main_cpp_CFLAGS = -DDATADIR=\"$(pkgdatadir)\"

DATA = azr3/presets

EXTRA_DIST = AUTHORS COPYING README ChangeLog


# Do the magic
include Makefile.template
