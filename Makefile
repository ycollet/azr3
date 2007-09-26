PACKAGE_NAME = azr3-lv2
PACKAGE_VERSION = 0.1.290
PKG_DEPS = gtkmm-2.4>=2.8.8


LV2_BUNDLES = mrvalve.lv2 azr3.lv2 azr3_gtk.lv2


# Mr Valve
mrvalve_lv2_MODULES = mrvalve.so
mrvalve_lv2_DATA = manifest.ttl mrvalve.ttl
mrvalve_lv2_PEGFILES = mrvalve.peg
mrvalve_lv2_SOURCEDIR = mrvalve
mrvalve_so_SOURCES = mrvalve.cpp
mrvalve_so_CFLAGS = -I. -Icommon

# AZR-3
azr3_lv2_MODULES = azr3.so
azr3_lv2_DATA = manifest.ttl azr3.ttl presets.ttl icon.svg
azr3_lv2_SOURCEDIR = azr3
azr3_so_SOURCES = \
	azr3.cpp azr3.hpp \
	Globals.h \
	fx.h fx.cpp \
	voice_classes.h voice_classes.cpp \
	programlist.hpp
azr3_so_CFLAGS = -I. -Icommon

# AZR-3 GUI
azr3_gtk_lv2_MODULES = azr3_gtk.so
azr3_gtk_lv2_SOURCEDIR = azr3
azr3_gtk_lv2_MANIFEST = gui_manifest.ttl
azr3_gtk_so_SOURCES = \
	azr3_gtk.cpp \
	knob.hpp knob.cpp \
	switch.hpp switch.cpp \
	drawbar.hpp drawbar.cpp \
	textbox.hpp textbox.cpp \
	cknob.xpm minioffon.xpm onoffgreen.xpm panelfx.xpm vonoff.xpm voice.xpm num_yellow.xpm dbblack.xpm dbbrown.xpm dbwhite.xpm
azr3_gtk_so_CFLAGS = `pkg-config --cflags gtkmm-2.4` -Iextensions/gtkgui -I. -Icommon
azr3_gtk_so_LDFLAGS = `pkg-config --libs gtkmm-2.4` 


# The shared headers need to go in the distribution too
EXTRA_DIST = AUTHORS COPYING README \
	common/lv2.h \
	common/lv2-gtk2gui.h \
	common/lv2-midiport.h \
	common/distortion.hpp \
	common/filters.hpp


# Do the magic
include Makefile.template
