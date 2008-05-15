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

#include <cassert>
#include <iomanip>
#include <sstream>

#include <gtkmm.h>

#include "azr3gui.hpp"


using namespace Gtk;
using namespace Gdk;
using namespace Glib;
using namespace std;
using namespace sigc;


AZR3GUI::AZR3GUI()
  : m_showing_fx_controls(true),
    m_current_program(0),
    m_splitkey(0),
    m_adj(kNumControls, 0) {
  
  m_fbox.set_has_window(true);
  m_vbox.set_has_window(true);
  m_fbox.put(m_vbox, 0, 615);
  m_voice_widgets.push_back(&m_vbox);
  
  RefPtr<Pixmap> pixmap = pixmap_from_file(DATADIR "/panelfx.png");
  RefPtr<Pixmap> voicepxm = pixmap_from_file(DATADIR "/voice.png");
  
  int w, h;
  pixmap->get_size(w, h);
  m_fbox.set_size_request(w, h);
  voicepxm->get_size(w, h);
  m_vbox.set_size_request(w, h);
  RefPtr<Style> s = m_fbox.get_style()->copy();
  s->set_bg_pixmap(STATE_NORMAL, pixmap);
  s->set_bg_pixmap(STATE_ACTIVE, pixmap);
  s->set_bg_pixmap(STATE_PRELIGHT, pixmap);
  s->set_bg_pixmap(STATE_SELECTED, pixmap);
  s->set_bg_pixmap(STATE_INSENSITIVE, pixmap);
  m_fbox.set_style(s);
  s = m_vbox.get_style()->copy();
  s->set_bg_pixmap(STATE_NORMAL, voicepxm);
  s->set_bg_pixmap(STATE_ACTIVE, voicepxm);
  s->set_bg_pixmap(STATE_PRELIGHT, voicepxm);
  s->set_bg_pixmap(STATE_SELECTED, voicepxm);
  s->set_bg_pixmap(STATE_INSENSITIVE, voicepxm);
  m_vbox.set_style(s);

  // add the display
  m_tbox = add_textbox(m_fbox, pixmap, 391, 19, 3, 140, 39);
  m_tbox->add_events(SCROLL_MASK);
  m_tbox->signal_scroll_display.
    connect(mem_fun(*this, &AZR3GUI::display_scroll));
  Menu* menu = create_menu();
  m_tbox->signal_button_press_event().
    connect(bind(mem_fun(*this, &AZR3GUI::popup_menu), menu));
  m_splitpoint_adj = new Adjustment(0, 0, 1);
  m_adj[n_splitpoint] = m_splitpoint_adj;
  m_splitpoint_adj->signal_value_changed().
    connect(mem_fun(*this, &AZR3GUI::splitpoint_changed));
  
  // keyboard split switch
  m_splitswitch = add_switch(m_fbox, -1, 537, 49, Switch::Mini);
  m_splitswitch->get_adjustment().signal_value_changed().
    connect(mem_fun(*this, &AZR3GUI::splitbox_clicked));
  
  // upper knobs
  add_switch(m_fbox, n_mono, 61, 105, Switch::Mini);
  add_knob(m_fbox, pixmap, n_click, 0, 1, 0.5, 88, 88, 0, 100, false);
  add_knob(m_fbox, pixmap, n_bender, 0, 1, 0.5, 132, 88, 0, 12, false);
  add_knob(m_fbox, pixmap, n_sustain, 0, 1, 0.5, 176, 88, 0, 100, false);
  add_knob(m_fbox, pixmap, n_shape, 0, 1, 0.5, 220, 88, 1, 6, false);
  add_knob(m_fbox, pixmap, n_perc, 0, 1, 0.5, 308, 88, 0, 10, false);
  add_knob(m_fbox, pixmap, n_percvol, 0, 1, 0.5, 352, 88, 0, 100, false);
  add_knob(m_fbox, pixmap, n_percfade, 0, 1, 0.5, 396, 88, 0, 100, false);
  add_knob(m_fbox, pixmap, n_vol1, 0, 1, 0.5, 484, 88, 0, 100, false);
  add_knob(m_fbox, pixmap, n_vol2, 0, 1, 0.5, 528, 88, 0, 100, false);
  add_knob(m_fbox, pixmap, n_vol3, 0, 1, 0.5, 572, 88, 0, 100, false);
  add_knob(m_fbox, pixmap, n_master, 0, 1, 0.5, 616, 88, 0, 100, false);

  // perc and sustain switches
  add_switch(m_fbox, n_1_perc, 16, 173, Switch::Mini);
  add_switch(m_fbox, n_2_perc, 279, 173, Switch::Mini);
  add_switch(m_fbox, n_3_perc, 542, 173, Switch::Mini);
  add_switch(m_fbox, n_1_sustain, 16, 214, Switch::Mini);
  add_switch(m_fbox, n_2_sustain, 279, 214, Switch::Mini);
  add_switch(m_fbox, n_3_sustain, 542, 214, Switch::Mini);
  
  // drawbars for channel 1
  add_drawbar(m_fbox, pixmap, n_1_db1, 0, 1, 0.5, 42, 159, Drawbar::Brown);
  add_drawbar(m_fbox, pixmap, n_1_db2, 0, 1, 0.5, 66, 159, Drawbar::Brown);
  add_drawbar(m_fbox, pixmap, n_1_db3, 0, 1, 0.5, 90, 159, Drawbar::White);
  add_drawbar(m_fbox, pixmap, n_1_db4, 0, 1, 0.5, 114, 159, Drawbar::White);
  add_drawbar(m_fbox, pixmap, n_1_db5, 0, 1, 0.5, 138, 159, Drawbar::Black);
  add_drawbar(m_fbox, pixmap, n_1_db6, 0, 1, 0.5, 162, 159, Drawbar::White);
  add_drawbar(m_fbox, pixmap, n_1_db7, 0, 1, 0.5, 186, 159, Drawbar::Black);
  add_drawbar(m_fbox, pixmap, n_1_db8, 0, 1, 0.5, 210, 159, Drawbar::Black);
  add_drawbar(m_fbox, pixmap, n_1_db9, 0, 1, 0.5, 234, 159, Drawbar::White);
  
  // drawbars for channel 2
  add_drawbar(m_fbox, pixmap, n_2_db1, 0, 1, 0.5, 306, 159, Drawbar::Brown);
  add_drawbar(m_fbox, pixmap, n_2_db2, 0, 1, 0.5, 330, 159, Drawbar::Brown);
  add_drawbar(m_fbox, pixmap, n_2_db3, 0, 1, 0.5, 354, 159, Drawbar::White);
  add_drawbar(m_fbox, pixmap, n_2_db4, 0, 1, 0.5, 378, 159, Drawbar::White);
  add_drawbar(m_fbox, pixmap, n_2_db5, 0, 1, 0.5, 402, 159, Drawbar::Black);
  add_drawbar(m_fbox, pixmap, n_2_db6, 0, 1, 0.5, 426, 159, Drawbar::White);
  add_drawbar(m_fbox, pixmap, n_2_db7, 0, 1, 0.5, 450, 159, Drawbar::Black);
  add_drawbar(m_fbox, pixmap, n_2_db8, 0, 1, 0.5, 474, 159, Drawbar::Black);
  add_drawbar(m_fbox, pixmap, n_2_db9, 0, 1, 0.5, 498, 159, Drawbar::White);

  // drawbars for channel 3
  add_drawbar(m_fbox, pixmap, n_3_db1, 0, 1, 0.5, 570, 159, Drawbar::Brown);
  add_drawbar(m_fbox, pixmap, n_3_db2, 0, 1, 0.5, 594, 159, Drawbar::Brown);
  add_drawbar(m_fbox, pixmap, n_3_db3, 0, 1, 0.5, 618, 159, Drawbar::White);
  add_drawbar(m_fbox, pixmap, n_3_db4, 0, 1, 0.5, 642, 159, Drawbar::White);
  add_drawbar(m_fbox, pixmap, n_3_db5, 0, 1, 0.5, 666, 159, Drawbar::Black);
  
  // mode switcher
  Widget* eb = add_clickbox(m_fbox, 14, 319, 14, 44);
  eb->signal_button_press_event().
    connect(sigc::hide(bind(bind(mem_fun(*this, &AZR3GUI::change_mode), 
				 ref(m_fbox)), false)));
  m_fx_widgets.push_back(eb);
  
  // Mr Valve controls
  m_fx_widgets.push_back(add_switch(m_fbox, n_mrvalve,
				  39, 332, Switch::Green));
  m_fx_widgets.push_back(add_knob(m_fbox, pixmap, n_drive, 
				  0, 1, 0.5, 44, 352, 0, 100, false));
  m_fx_widgets.push_back(add_knob(m_fbox, pixmap, n_set, 
				  0, 1, 0.5, 88, 352, 0, 100, false));
  m_fx_widgets.push_back(add_knob(m_fbox, pixmap, n_tone, 0, 
				  1, 0.5, 132, 352, 300, 3500, false));
  m_fx_widgets.push_back(add_knob(m_fbox, pixmap, n_mix, 0, 1, 
				  0.5, 176, 352, 0, 100, false));
  
  // Speaker controls
  m_fx_widgets.push_back(add_switch(m_fbox, n_speakers, 
				  302, 332, Switch::Green));
  m_fx_widgets.push_back(add_switch(m_fbox, n_speed, 
				  323, 356, Switch::BigRed));
  m_fx_widgets.push_back(add_knob(m_fbox, pixmap, n_l_slow, 
				  0, 1, 0.5, 352, 352, 0, 10, true));
  m_fx_widgets.push_back(add_knob(m_fbox, pixmap, n_l_fast, 
				  0, 1, 0.5, 396, 352, 0, 10, true));
  m_fx_widgets.push_back(add_knob(m_fbox, pixmap, n_u_slow, 
				  0, 1, 0.5, 440, 352, 0, 10, true));
  m_fx_widgets.push_back(add_knob(m_fbox, pixmap, n_u_fast, 
				  0, 1, 0.5, 484, 352, 0, 10, true));
  m_fx_widgets.push_back(add_knob(m_fbox, pixmap, n_belt, 
				  0, 1, 0.5, 528, 352, 0, 100, false));
  m_fx_widgets.push_back(add_knob(m_fbox, pixmap, n_spread, 
				  0, 1, 0.5, 572, 352, 0, 100, false));
  m_fx_widgets.push_back(add_switch(m_fbox, n_complex, 
				    443, 331, Switch::Mini));
  m_fx_widgets.push_back(add_switch(m_fbox, n_pedalspeed, 
				    510, 331, Switch::Mini));

  // mode switcher 2
  Widget* eb2 = add_clickbox(m_vbox, 14, 53, 14, 44);
  eb2->signal_button_press_event().
    connect(sigc::hide(bind(bind(mem_fun(*this, &AZR3GUI::change_mode), 
				 ref(m_fbox)), true)));

  // vibrato controls
  add_switch(m_vbox, n_1_vibrato, 39, 17, Switch::Green);
  add_knob(m_vbox, voicepxm, n_1_vstrength, 0, 1, 0.5, 88, 37, 0, 100, false);
  add_knob(m_vbox, voicepxm, n_1_vmix, 0, 1, 0.5, 176, 37, 0, 100, false);
  add_switch(m_vbox, n_2_vibrato, 302, 17, Switch::Green);
  add_knob(m_vbox, voicepxm, n_2_vstrength, 0, 1, 0.5, 352, 37, 0, 100, false);
  add_knob(m_vbox, voicepxm, n_2_vmix, 0, 1, 0.5, 440, 37, 0, 100, false);
  
  pack_start(m_fbox);
    
  splitpoint_changed();
    
}
  

void AZR3GUI::set_control(uint32_t port, float value) {
  if (port < m_adj.size() && m_adj[port])
    m_adj[port]->set_value(value);
}
  
  

void AZR3GUI::add_program(unsigned char number, const char* name) {
  m_programs[number] = name;
  update_program_menu();
}



void AZR3GUI::remove_program(unsigned char number) {
  std::map<int, string>::iterator iter = m_programs.find(number);
  if (iter != m_programs.end()) {
    m_programs.erase(iter);
    update_program_menu();
  }
}
  
  

void AZR3GUI::set_program(unsigned char number) {
  std::map<int, string>::const_iterator iter = m_programs.find(number);
  ostringstream oss;
  oss<<"AZR-3 JACK P"<<setw(2)<<setfill('0')<<int(number);
  m_tbox->set_string(0, oss.str());
  if (iter != m_programs.end())
    m_tbox->set_string(1, iter->second.substr(0, 23));
  m_current_program = number;
}



void AZR3GUI::clear_programs() {
  if (m_programs.size() > 0) {
    m_programs.clear();
    update_program_menu();
  }
}


Glib::RefPtr<Gdk::Pixmap> AZR3GUI::pixmap_from_file(const std::string& file, RefPtr<Bitmap>* bitmap) {
  RefPtr<Pixbuf> pixbuf = Pixbuf::create_from_file(file);
  RefPtr<Pixmap> pixmap;
  if (bitmap)
    pixbuf->render_pixmap_and_mask(pixmap, *bitmap, 255);
  else {
    RefPtr<Bitmap> tmp;
    pixbuf->render_pixmap_and_mask(pixmap, tmp, 255);
  }
  return pixmap;
}

  
void AZR3GUI::control_changed(uint32_t index, float new_value) {
  signal_set_control(index, new_value);
}

  
void AZR3GUI::program_changed(int program) {
  signal_set_program(program);
}


void AZR3GUI::on_realize() {
  HBox::on_realize();
}

  
string AZR3GUI::note2str(long note) {
  static char notestr[4];
  int octave = int(note / 12);
  switch(note % 12)
    {
    case 0:
      sprintf(notestr, "C %1d", octave);
      break;
    case 1:
      sprintf(notestr, "C#%1d", octave);
      break;
    case 2:
      sprintf(notestr, "D %1d", octave);
      break;
    case 3:
      sprintf(notestr, "D#%1d", octave);
      break;
    case 4:
      sprintf(notestr, "E %1d", octave);
      break;
    case 5:
      sprintf(notestr, "F %1d", octave);
      break;
    case 6:
      sprintf(notestr, "F#%1d", octave);
      break;
    case 7:
      sprintf(notestr, "G %1d", octave);
      break;
    case 8:
      sprintf(notestr, "G#%1d", octave);
      break;
    case 9:
      sprintf(notestr, "A %1d", octave);
      break;
    case 10:
      sprintf(notestr, "A#%1d", octave);
      break;
    case 11:
      sprintf(notestr, "B %1d", octave);
      break;
    }
  return string(notestr);
}
  
  
void AZR3GUI::splitbox_clicked() {
  if (m_splitswitch->get_adjustment().get_value() < 0.5)
    m_splitpoint_adj->set_value(0);
  else
    m_splitpoint_adj->set_value(m_splitkey / 128.0);
}


void AZR3GUI::set_back_pixmap(Widget* wdg, RefPtr<Pixmap> pm) {
  wdg->get_window()->set_back_pixmap(pm, false);
}


Knob* AZR3GUI::add_knob(Fixed& fbox, RefPtr<Pixmap>& pm, int port, 
			float min, float max, float value, 
			int xoffset, int yoffset,
			float dmin, float dmax, bool decimal) {
  Knob* knob = manage(new Knob(min, max, value, dmin, dmax, decimal));
  fbox.put(*knob, xoffset, yoffset);
  int w, h;
  knob->get_size_request(w, h);
  RefPtr<Style> s = knob->get_style()->copy();
  RefPtr<Pixmap> npm = Pixmap::create(pm, w, h);
  RefPtr<GC> gc = GC::create(npm);
  npm->draw_drawable(gc, pm, xoffset, yoffset, 0, 0, w, h);
  s->set_bg_pixmap(STATE_NORMAL, npm);
  s->set_bg_pixmap(STATE_ACTIVE, npm);
  s->set_bg_pixmap(STATE_PRELIGHT, npm);
  s->set_bg_pixmap(STATE_SELECTED, npm);
  s->set_bg_pixmap(STATE_INSENSITIVE, npm);
  knob->set_style(s);
  if (port >= 0 && port < m_adj.size()) {
    knob->get_adjustment().signal_value_changed().
      connect(compose(bind<0>(mem_fun(*this, &AZR3GUI::control_changed), port),
		      mem_fun(knob->get_adjustment(), 
			      &Adjustment::get_value)));
    assert(m_adj[port] == 0);
    m_adj[port] = &knob->get_adjustment();
  }
  return knob;
}


Drawbar* AZR3GUI::add_drawbar(Fixed& fbox, RefPtr<Pixmap>& pm, int port, 
			      float min, float max, float value, 
			      int xoffset, int yoffset, 
			      Drawbar::Type type) {
  Drawbar* db = manage(new Drawbar(min, max, value, type));
  fbox.put(*db, xoffset, yoffset);
  int w, h;
  db->get_size_request(w, h);
  RefPtr<Style> s = db->get_style()->copy();
  RefPtr<Pixmap> npm = Pixmap::create(pm, w, h);
  RefPtr<GC> gc = GC::create(npm);
  npm->draw_drawable(gc, pm, xoffset, yoffset, 0, 0, w, h);
  s->set_bg_pixmap(STATE_NORMAL, npm);
  s->set_bg_pixmap(STATE_ACTIVE, npm);
  s->set_bg_pixmap(STATE_PRELIGHT, npm);
  s->set_bg_pixmap(STATE_SELECTED, npm);
  s->set_bg_pixmap(STATE_INSENSITIVE, npm);
  db->set_style(s);
  if (port >= 0 && port < m_adj.size()) {
    db->get_adjustment().signal_value_changed().
      connect(compose(bind<0>(mem_fun(*this, &AZR3GUI::control_changed), port),
		      mem_fun(db->get_adjustment(), &Adjustment::get_value)));
    assert(m_adj[port] == 0);
    m_adj[port] = &db->get_adjustment();
  }
  return db;
}


Switch* AZR3GUI::add_switch(Fixed& fbox, int port,
			    int xoffset, int yoffset, Switch::Type type) {
  Switch* sw = manage(new Switch(type));
  fbox.put(*sw, xoffset, yoffset);
  if (port >= 0 && port < m_adj.size()) {
    sw->get_adjustment().signal_value_changed().
      connect(compose(bind<0>(mem_fun(*this, &AZR3GUI::control_changed),port),
		      mem_fun(sw->get_adjustment(), &Adjustment::get_value)));
    assert(m_adj[port] == 0);
    m_adj[port] = &sw->get_adjustment();
  }
  return sw;
}


EventBox* AZR3GUI::add_clickbox(Fixed& fbox, int xoffset, int yoffset, 
				int width, int height) {
  EventBox* eb = manage(new EventBox);
  eb->set_events(BUTTON_PRESS_MASK);
  eb->set_size_request(width, height);
  fbox.put(*eb, xoffset, yoffset);
  eb->set_visible_window(false);
  return eb;
}


Textbox* AZR3GUI::add_textbox(Fixed& fbox, RefPtr<Pixmap>& pm,
			      int xoffset, int yoffset, int lines, 
			      int width, int height) {
  Textbox* db = manage(new Textbox(width, height, lines));
  fbox.put(*db, xoffset, yoffset);
  int w, h;
  db->get_size_request(w, h);
  RefPtr<Style> s = db->get_style()->copy();
  RefPtr<Pixmap> npm = Pixmap::create(pm, w, h);
  RefPtr<GC> gc = GC::create(npm);
  npm->draw_drawable(gc, pm, xoffset, yoffset, 0, 0, w, h);
  s->set_bg_pixmap(STATE_NORMAL, npm);
  s->set_bg_pixmap(STATE_ACTIVE, npm);
  s->set_bg_pixmap(STATE_PRELIGHT, npm);
  s->set_bg_pixmap(STATE_SELECTED, npm);
  s->set_bg_pixmap(STATE_INSENSITIVE, npm);
  db->set_style(s);
  return db;
}


bool AZR3GUI::change_mode(bool fx_mode, Fixed& fbox) {
  int x, y;
  if (fx_mode && !m_showing_fx_controls) {
    for (unsigned i = 0; i < m_voice_widgets.size(); ++i) {
      m_voice_widgets[i]->translate_coordinates(m_fbox, 0, 0, x, y);
      m_fbox.move(*m_voice_widgets[i], x, y + 300);
    }
    for (unsigned i = 0; i < m_fx_widgets.size(); ++i) {
      m_fx_widgets[i]->translate_coordinates(m_fbox, 0, 0, x, y);
      m_fbox.move(*m_fx_widgets[i], x, y - 300);
    }
  }
  else if (!fx_mode && m_showing_fx_controls) {
    for (unsigned i = 0; i < m_fx_widgets.size(); ++i) {
      m_fx_widgets[i]->translate_coordinates(m_fbox, 0, 0, x, y);
      m_fbox.move(*m_fx_widgets[i], x, y + 300);
    }
    for (unsigned i = 0; i < m_voice_widgets.size(); ++i) {
      m_voice_widgets[i]->translate_coordinates(m_fbox, 0, 0, x, y);
      m_fbox.move(*m_voice_widgets[i], x, y - 300);
    }
  }
  m_showing_fx_controls = fx_mode;
  return true;
}


void AZR3GUI::splitpoint_changed() {
  float value = m_adj[n_splitpoint]->get_value();
  int key = int(value * 128);
  if (key <= 0 || key >= 128) {
    m_tbox->set_string(2, "Keyboard split OFF");
    m_splitswitch->get_adjustment().set_value(0);
  }
  else {
    m_splitkey = key;
    m_tbox->set_string(2, string("Splitpoint: ") + note2str(key));
    m_splitswitch->get_adjustment().set_value(1);
  }
}


void AZR3GUI::update_program_menu() {
  m_program_menu->items().clear();
  std::map<int, string>::const_iterator iter;
  for (iter = m_programs.begin(); iter != m_programs.end(); ++iter) {
    ostringstream oss;
    oss<<setw(2)<<setfill('0')<<iter->first<<' '<<iter->second.substr(0, 23);
    MenuItem* item = manage(new MenuItem(oss.str()));
    item->signal_activate().
      connect(bind(mem_fun(*this, &AZR3GUI::program_changed), iter->first));
    m_program_menu->items().push_back(*item);
    item->show();
    item->get_child()->modify_bg(STATE_NORMAL, m_menu_bg);
    item->get_child()->modify_fg(STATE_NORMAL, m_menu_fg);
  }
}


Menu* AZR3GUI::create_menu() {
  
  m_menu_bg.set_rgb(16000, 16000, 16000);
  m_menu_fg.set_rgb(65535, 65535, 50000);
  Menu* menu = manage(new Menu);
  
  m_program_menu = manage(new Menu);
  update_program_menu();
  
  MenuItem* program_item = manage(new MenuItem("Select program"));
  program_item->set_submenu(*m_program_menu);
  program_item->show();
  program_item->get_child()->modify_fg(STATE_NORMAL, m_menu_bg);
  program_item->get_child()->modify_fg(STATE_NORMAL, m_menu_fg);
  
  MenuItem* save_item = manage(new MenuItem("Save program"));
  save_item->signal_activate().
    connect(mem_fun(*this, &AZR3GUI::save_program));
  save_item->show();
  save_item->get_child()->modify_fg(STATE_NORMAL, m_menu_bg);
  save_item->get_child()->modify_fg(STATE_NORMAL, m_menu_fg);
  
  menu->items().push_back(*program_item);
  menu->items().push_back(*save_item);
  
  menu->modify_bg(STATE_NORMAL, m_menu_bg);
  menu->modify_fg(STATE_NORMAL, m_menu_fg);
  m_program_menu->modify_bg(STATE_NORMAL, m_menu_bg);
  m_program_menu->modify_fg(STATE_NORMAL, m_menu_fg);
  
  return menu;
}


bool AZR3GUI::popup_menu(GdkEventButton* event, Menu* menu) {
  menu->popup(event->button, event->time);
  return true;
}


void AZR3GUI::display_scroll(int line, GdkEventScroll* e) {
  if (line < 2) {
    std::map<int, string>::const_iterator iter = 
      m_programs.find(m_current_program);
    if (iter == m_programs.end())
      iter = m_programs.begin();
    if (iter != m_programs.end()) {
      if (e->direction == GDK_SCROLL_UP) {
	++iter;
	iter = (iter == m_programs.end() ? m_programs.begin() : iter);
	signal_set_program(iter->first);
      }
      else if (e->direction == GDK_SCROLL_DOWN) {
	if (iter == m_programs.begin())
	  for (unsigned i = 0; i < m_programs.size() - 1; ++i, ++iter);
	else
	  --iter;
	signal_set_program(iter->first);
      }
    }
  }
  else {
    int oldsplitkey = m_splitkey;
    if (m_splitswitch->get_adjustment().get_value() < 0.5)
      m_splitkey = 0;
    if (e->direction == GDK_SCROLL_UP)
      m_splitkey = (m_splitkey + 1) % 128;
    else if (e->direction == GDK_SCROLL_DOWN)
      m_splitkey = (m_splitkey - 1) % 128;
    while (m_splitkey < 0)
      m_splitkey += 128;
    if (oldsplitkey != m_splitkey)
      m_splitpoint_adj->set_value(m_splitkey / 128.0);
  }
}


void AZR3GUI::save_program() {
  Dialog dlg("Save program");
  dlg.add_button(Stock::CANCEL, RESPONSE_CANCEL);
  dlg.add_button(Stock::OK, RESPONSE_OK);
  Table tbl(2, 2);
  tbl.set_col_spacings(3);
  tbl.set_row_spacings(3);
  tbl.set_border_width(3);
  Label lbl1("Name:");
  Label lbl2("Number:");
  Entry ent;
  ent.set_max_length(23);
  Adjustment adj(0, 0, 127);
  SpinButton spb(adj);
  spb.set_value(m_current_program);
  tbl.attach(lbl1, 0, 1, 0, 1);
  tbl.attach(lbl2, 0, 1, 1, 2);
  tbl.attach(ent, 1, 2, 0, 1);
  tbl.attach(spb, 1, 2, 1, 2);
  dlg.get_vbox()->pack_start(tbl);
  dlg.show_all();
  while (dlg.run() == RESPONSE_OK) {
    if (m_programs.find((unsigned char)adj.get_value()) != m_programs.end()) {
      MessageDialog msg("There is already a program with this number. Are "
			"you sure that you want to overwrite it?", false,
			MESSAGE_QUESTION, BUTTONS_YES_NO);
      msg.show_all();
      if (msg.run() == RESPONSE_NO)
	continue;
    }
    signal_save_program(adj.get_value(), ent.get_text());
    break;
  }
}


