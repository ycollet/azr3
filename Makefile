PACKAGE_NAME = azr3-lv2
PACKAGE_VERSION = 0.1.281
PKG_DEPS = gtkmm-2.4>=2.8.8


LV2_PLUGINS = mrvalve.lv2 azr3.lv2


# Mr Valve
mrvalve_lv2_SOURCES = mrvalve.cpp
mrvalve_lv2_DATA = manifest.ttl mrvalve.ttl
mrvalve_lv2_PEGFILES = mrvalve.peg
mrvalve_lv2_CFLAGS = -I. -Icommon
mrvalve_lv2_SOURCEDIR = mrvalve

# AZR-3
azr3_lv2_SOURCES = \
	azr3.cpp azr3.hpp \
	Globals.h \
	fx.h fx.cpp \
	voice_classes.h voice_classes.cpp \
	programlist.hpp
azr3_lv2_DATA = manifest.ttl azr3.ttl presets.ttl icon.svg
azr3_lv2_CFLAGS = -I. -Icommon
azr3_lv2_SOURCEDIR = azr3
azr3_lv2_MODULES = azr3_gtk.so
azr3_gtk_so_SOURCES = \
	azr3_gtk.cpp \
	knob.hpp knob.cpp \
	switch.hpp switch.cpp \
	drawbar.hpp drawbar.cpp \
	textbox.hpp textbox.cpp \
	cknob.xpm minioffon.xpm onoffgreen.xpm panelfx.xpm vonoff.xpm voice.xpm num_yellow.xpm dbblack.xpm dbbrown.xpm dbwhite.xpm
azr3_gtk_so_CFLAGS = `pkg-config --cflags gtkmm-2.4` -Iextensions/gtkgui -I. -Icommon
azr3_gtk_so_LDFLAGS = `pkg-config --libs gtkmm-2.4` 
azr3_gtk_so_SOURCEDIR = azr3


# The shared headers need to go in the distribution too
EXTRA_DIST = AUTHORS COPYING README \
	common/lv2.h \
	common/lv2-gtk2gui.h \
	common/lv2-midiport.h \
	common/distortion.hpp \
	common/filters.hpp


# Do the magic
include Makefile.template
