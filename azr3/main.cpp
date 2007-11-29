#include <gtkmm.h>

#include "azr3.hpp"
#include "azr3_gtk.hpp"


int main(int argc, char** argv) {

  Gtk::Main kit(argc, argv);
  Gtk::Window win;
  AZR3 engine(48000);
  AZR3GUI gui;
  
  win.set_title("AZR-3");
  win.add(gui);
  win.show_all();

  kit.run(win);
  
  return 0;
}
