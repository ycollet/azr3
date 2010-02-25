PACKAGE_NAME = azr3-jack
PACKAGE_VERSION = $(shell ./VERSION)
PACKAGE_DISPLAY_NAME = AZR3-JACK
define PACKAGE_DESCRIPTION
This JACK program is a port of the free VST plugin AZR-3. It is a
tonewheel organ with drawbars, distortion and rotating speakers.
The original was written by Rumpelrausch Täips.
endef
PACKAGE_WEBPAGE = "http://ll-plugins.nongnu.org/azr3/"
PACKAGE_BUGTRACKER = "https://savannah.nongnu.org/bugs/?group=ll-plugins"
PACKAGE_VC = "http://git.savannah.gnu.org/cgit/ll-plugins/azr3-jack.git/"

PKG_DEPS = gtkmm-2.4>=2.8.8 jack>=0.103.0 lash-1.0>=0.5.3


PROGRAMS = azr3

MANUALS = azr3.1

azr3_SOURCES = \
	main.cpp main.hpp \
	azr3.cpp azr3.hpp \
	globals.hpp \
	filters.hpp \
	fx.hpp fx.cpp \
	newjack.hpp \
	optionparser.cpp optionparser.hpp \
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
main_cpp_CFLAGS = -DPACKAGE_VERSION=\"$(PACKAGE_VERSION)\"

DATA = \
	azr3/presets \
	azr3/cknob.png azr3/minioffon.png azr3/onoffgreen.png azr3/panelfx.png azr3/vonoff.png azr3/voice.png azr3/num_yellow.png azr3/dbblack.png azr3/dbbrown.png azr3/dbwhite.png

DOCS = AUTHORS COPYING README ChangeLog


# Do the magic
include Makefile.template
