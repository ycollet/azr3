/****************************************************************************
    
    AZR-3 - An organ synth
    
    Copyright (C) 2006-2007 Lars Luthman <lars.luthman@gmail.com>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as published
    by the Free Software Foundation.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA

****************************************************************************/

#ifndef AZR3_GTK_HPP
#define AZR3_GTK_HPP

#include <stdint.h>
#include <vector>

#include <gtkmm.h>

#include "drawbar.hpp"
#include "knob.hpp"
#include "switch.hpp"
#include "globals.hpp"
#include "textbox.hpp"


class AZR3GUI : public Gtk::HBox {
public:
  
  sigc::signal<void, uint32_t, float> signal_set_control;
  sigc::signal<void, unsigned char> signal_set_program;
  sigc::signal<void, unsigned char, std::string> signal_save_program;
  
  AZR3GUI();
  
  void set_control(uint32_t port, float value);
  void add_program(unsigned char number, const char* name);
  void remove_program(unsigned char number);
  void set_program(unsigned char number);
  void clear_programs();
  
  static Glib::RefPtr<Gdk::Pixmap> pixmap_from_file(const std::string& file, Glib::RefPtr<Gdk::Bitmap>* bitmap = 0);
  
protected:

  void control_changed(uint32_t index, float new_value);
  void program_changed(int program);
  
  void on_realize();
  
  static std::string note2str(long note);
  
  void splitbox_clicked();
  void set_back_pixmap(Widget* wdg, Glib::RefPtr<Gdk::Pixmap> pm);
  Knob* add_knob(Gtk::Fixed& fbox, Glib::RefPtr<Gdk::Pixmap>& pm, size_t port, 
                 float min, float max, float value, 
                 int xoffset, int yoffset,
                 float dmin, float dmax, bool decimal);
  Drawbar* add_drawbar(Gtk::Fixed& fbox, Glib::RefPtr<Gdk::Pixmap>& pm,
		       size_t port, float min, float max, float value, 
                       int xoffset, int yoffset, 
                       Drawbar::Type type);
  Switch* add_switch(Gtk::Fixed& fbox, size_t port,
                     int xoffset, int yoffset, Switch::Type type);
  Gtk::EventBox* add_clickbox(Gtk::Fixed& fbox, int xoffset, int yoffset, 
			      int width, int height);
  Textbox* add_textbox(Gtk::Fixed& fbox, Glib::RefPtr<Gdk::Pixmap>& pm,
                       int xoffset, int yoffset, int lines, 
                       int width, int height);
  bool change_mode(bool fx_mode, Gtk::Fixed& fbox);
  void splitpoint_changed();
  void update_program_menu();
  void update_split_menu();
  Gtk::Menu* create_menu();
  bool popup_menu(GdkEventButton* event, Gtk::Menu* menu);
  void display_scroll(int line, GdkEventScroll* e);
  void save_program();


  bool m_showing_fx_controls;
  std::vector<Widget*> m_fx_widgets;
  std::vector<Widget*> m_voice_widgets;
  std::map<int, std::string> m_programs;
  int m_current_program;
  int m_splitkey;
  Textbox* m_tbox;
  Switch* m_splitswitch;
  Gtk::Adjustment* m_splitpoint_adj;
  Gtk::Menu* m_program_menu;
  Gtk::Menu* m_split_menu;
  Gdk::Color m_menu_bg;
  Gdk::Color m_menu_fg;
  Gtk::Fixed m_fbox;
  Gtk::Fixed m_vbox;
  std::vector<Gtk::Adjustment*> m_adj;

};


#endif
