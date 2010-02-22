PACKAGE_NAME = azr3-jack
PACKAGE_VERSION = $(shell git describe --match 'Version_*' | sed 's/Version_//' | sed 's/-/ /g' | awk '{ print $$1 " " $$2}' | sed -r 's/\.([0-9]+) / \1 /' | awk '{ print $$1 "." $$2+$$3 }')$(shell if test $$(git ls-files --modified | wc -l) -gt 0 ; then echo .EDITED; fi)
PKG_DEPS = gtkmm-2.4>=2.8.8 jack>=0.103.0 lash-1.0>=0.5.3


PROGRAMS = azr3

azr3_SOURCES = \
	main.cpp main.hpp \
	azr3.cpp azr3.hpp \
	globals.hpp \
	filters.hpp \
	fx.hpp fx.cpp \
	newjack.hpp \
	voice_classes.hpp voice_classes.cpp \
	azr3gui.cpp azr3gui.hpp \
	knob.hpp knob.cpp \
	switch.hpp switch.cpp \
	drawbar.hpp drawbar.cpp \
	textbox.hpp textbox.cpp
azr3_SOURCEDIR = azr3
azr3_CFLAGS = `pkg-config --cflags gtkmm-2.4 jack lash-1.0` -DDATADIR=\"$(pkgdatadir)\"
azr3_LDFLAGS = `pkg-config --libs gtkmm-2.4 jack lash-1.0`
azr3_cpp_CFLAGS = $(shell if pkg-config --atleast-version=0.107 jack ; then echo -include azr3/newjack.hpp; fi)

DATA = \
	azr3/presets \
	azr3/cknob.png azr3/minioffon.png azr3/onoffgreen.png azr3/panelfx.png azr3/vonoff.png azr3/voice.png azr3/num_yellow.png azr3/dbblack.png azr3/dbbrown.png azr3/dbwhite.png

DOCS = AUTHORS COPYING README ChangeLog


# Do the magic
include Makefile.template
