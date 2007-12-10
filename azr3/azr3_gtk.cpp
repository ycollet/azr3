/****************************************************************************
    
    AZR-3 - An LV2 synth plugin
    
    Copyright (C) 2006  Lars Luthman <lars.luthman@gmail.com>
    
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
#include <iostream>
#include <sstream>
#include <vector>

#include <gtkmm.h>

#include "azr3_gtk.hpp"
#include "panelfx.xpm"
#include "voice.xpm"
#include "drawbar.hpp"
#include "knob.hpp"
#include "switch.hpp"
#include "Globals.h"
#include "textbox.hpp"


using namespace Gtk;
using namespace Gdk;
using namespace Glib;
using namespace std;
using namespace sigc;


AZR3GUI::AZR3GUI()
  : showing_fx_controls(true),
    current_program(0),
    splitkey(0),
    m_adj(kNumControls, 0) {
  
  fbox.set_has_window(true);
  vbox.set_has_window(true);
  fbox.put(vbox, 0, 615);
  voice_widgets.push_back(&vbox);
  RefPtr<Pixbuf> voicebuf = Pixbuf::create_from_xpm_data(voice);
  RefPtr<Pixbuf> pixbuf = Pixbuf::create_from_xpm_data(panelfx);
  fbox.set_size_request(pixbuf->get_width(), pixbuf->get_height());
  vbox.set_size_request(voicebuf->get_width(), voicebuf->get_height());
  RefPtr<Pixmap> pixmap = Pixmap::create(fbox.get_window(), 
					 pixbuf->get_width(), 
					 pixbuf->get_height());
  RefPtr<Pixmap> voicepxm = Pixmap::create(vbox.get_window(),
					   voicebuf->get_width(),
					   voicebuf->get_height());
  
  RefPtr<Bitmap> bitmap;
  pixbuf->render_pixmap_and_mask(pixmap, bitmap, 10);
  voicebuf->render_pixmap_and_mask(voicepxm, bitmap, 10);
  RefPtr<Style> s = fbox.get_style()->copy();
  s->set_bg_pixmap(STATE_NORMAL, pixmap);
  s->set_bg_pixmap(STATE_ACTIVE, pixmap);
  s->set_bg_pixmap(STATE_PRELIGHT, pixmap);
  s->set_bg_pixmap(STATE_SELECTED, pixmap);
  s->set_bg_pixmap(STATE_INSENSITIVE, pixmap);
  fbox.set_style(s);
  s = vbox.get_style()->copy();
  s->set_bg_pixmap(STATE_NORMAL, voicepxm);
  s->set_bg_pixmap(STATE_ACTIVE, voicepxm);
  s->set_bg_pixmap(STATE_PRELIGHT, voicepxm);
  s->set_bg_pixmap(STATE_SELECTED, voicepxm);
  s->set_bg_pixmap(STATE_INSENSITIVE, voicepxm);
  vbox.set_style(s);
  
  // add the display
  tbox = add_textbox(fbox, pixmap, 391, 19, 3, 140, 39);
  tbox->add_events(SCROLL_MASK);
  tbox->signal_scroll_display.
    connect(mem_fun(*this, &AZR3GUI::display_scroll));
  Menu* menu = create_menu();
  tbox->signal_button_press_event().
    connect(bind(mem_fun(*this, &AZR3GUI::popup_menu), menu));
  splitpoint_adj = new Adjustment(0, 0, 1);
  m_adj[n_splitpoint] = splitpoint_adj;
  splitpoint_adj->signal_value_changed().
    connect(mem_fun(*this, &AZR3GUI::splitpoint_changed));
  
  // keyboard split switch
  splitswitch = add_switch(fbox, -1, 537, 49, Switch::Mini);
  splitswitch->get_adjustment().signal_value_changed().
    connect(mem_fun(*this, &AZR3GUI::splitbox_clicked));
  
  // upper knobs
  add_switch(fbox, n_mono, 61, 105, Switch::Mini);
  add_knob(fbox, n_click, 0, 1, 0.5, pixmap, 88, 88, 0, 100, false);
  add_knob(fbox, n_bender, 0, 1, 0.5, pixmap, 132, 88, 0, 12, false);
  add_knob(fbox, n_sustain, 0, 1, 0.5, pixmap, 176, 88, 0, 100, false);
  add_knob(fbox, n_shape, 0, 1, 0.5, pixmap, 220, 88, 1, 6, false);
  add_knob(fbox, n_perc, 0, 1, 0.5, pixmap, 308, 88, 0, 10, false);
  add_knob(fbox, n_percvol, 0, 1, 0.5, pixmap, 352, 88, 0, 100, false);
  add_knob(fbox, n_percfade, 0, 1, 0.5, pixmap, 396, 88, 0, 100, false);
  add_knob(fbox, n_vol1, 0, 1, 0.5, pixmap, 484, 88, 0, 100, false);
  add_knob(fbox, n_vol2, 0, 1, 0.5, pixmap, 528, 88, 0, 100, false);
  add_knob(fbox, n_vol3, 0, 1, 0.5, pixmap, 572, 88, 0, 100, false);
  add_knob(fbox, n_master, 0, 1, 0.5, pixmap, 616, 88, 0, 100, false);

  // perc and sustain switches
  add_switch(fbox, n_1_perc, 16, 173, Switch::Mini);
  add_switch(fbox, n_2_perc, 279, 173, Switch::Mini);
  add_switch(fbox, n_3_perc, 542, 173, Switch::Mini);
  add_switch(fbox, n_1_sustain, 16, 214, Switch::Mini);
  add_switch(fbox, n_2_sustain, 279, 214, Switch::Mini);
  add_switch(fbox, n_3_sustain, 542, 214, Switch::Mini);
  
  // drawbars for channel 1
  add_drawbar(fbox, n_1_db1, 0, 1, 0.5, pixmap, 42, 159, Drawbar::Brown);
  add_drawbar(fbox, n_1_db2, 0, 1, 0.5, pixmap, 66, 159, Drawbar::Brown);
  add_drawbar(fbox, n_1_db3, 0, 1, 0.5, pixmap, 90, 159, Drawbar::White);
  add_drawbar(fbox, n_1_db4, 0, 1, 0.5, pixmap, 114, 159, Drawbar::White);
  add_drawbar(fbox, n_1_db5, 0, 1, 0.5, pixmap, 138, 159, Drawbar::Black);
  add_drawbar(fbox, n_1_db6, 0, 1, 0.5, pixmap, 162, 159, Drawbar::White);
  add_drawbar(fbox, n_1_db7, 0, 1, 0.5, pixmap, 186, 159, Drawbar::Black);
  add_drawbar(fbox, n_1_db8, 0, 1, 0.5, pixmap, 210, 159, Drawbar::Black);
  add_drawbar(fbox, n_1_db9, 0, 1, 0.5, pixmap, 234, 159, Drawbar::White);
  
  // drawbars for channel 2
  add_drawbar(fbox, n_2_db1, 0, 1, 0.5, pixmap, 306, 159, Drawbar::Brown);
  add_drawbar(fbox, n_2_db2, 0, 1, 0.5, pixmap, 330, 159, Drawbar::Brown);
  add_drawbar(fbox, n_2_db3, 0, 1, 0.5, pixmap, 354, 159, Drawbar::White);
  add_drawbar(fbox, n_2_db4, 0, 1, 0.5, pixmap, 378, 159, Drawbar::White);
  add_drawbar(fbox, n_2_db5, 0, 1, 0.5, pixmap, 402, 159, Drawbar::Black);
  add_drawbar(fbox, n_2_db6, 0, 1, 0.5, pixmap, 426, 159, Drawbar::White);
  add_drawbar(fbox, n_2_db7, 0, 1, 0.5, pixmap, 450, 159, Drawbar::Black);
  add_drawbar(fbox, n_2_db8, 0, 1, 0.5, pixmap, 474, 159, Drawbar::Black);
  add_drawbar(fbox, n_2_db9, 0, 1, 0.5, pixmap, 498, 159, Drawbar::White);

  // drawbars for channel 3
  add_drawbar(fbox, n_3_db1, 0, 1, 0.5, pixmap, 570, 159, Drawbar::Brown);
  add_drawbar(fbox, n_3_db2, 0, 1, 0.5, pixmap, 594, 159, Drawbar::Brown);
  add_drawbar(fbox, n_3_db3, 0, 1, 0.5, pixmap, 618, 159, Drawbar::White);
  add_drawbar(fbox, n_3_db4, 0, 1, 0.5, pixmap, 642, 159, Drawbar::White);
  add_drawbar(fbox, n_3_db5, 0, 1, 0.5, pixmap, 666, 159, Drawbar::Black);
  
  // mode switcher
  Widget* eb = add_clickbox(fbox, 14, 319, 14, 44);
  eb->signal_button_press_event().
    connect(sigc::hide(bind(bind(mem_fun(*this, &AZR3GUI::change_mode), 
				 ref(fbox)), false)));
  fx_widgets.push_back(eb);
  
  // Mr Valve controls
  fx_widgets.push_back(add_switch(fbox, n_mrvalve,
				  39, 332, Switch::Green));
  fx_widgets.push_back(add_knob(fbox, n_drive, 
				0, 1, 0.5, pixmap, 44, 352, 0, 100, false));
  fx_widgets.push_back(add_knob(fbox, n_set, 
				0, 1, 0.5, pixmap, 88, 352, 0, 100, false));
  fx_widgets.push_back(add_knob(fbox, n_tone, 0, 
				1, 0.5, pixmap, 132, 352, 300, 3500, false));
  fx_widgets.push_back(add_knob(fbox, n_mix, 0, 1, 
				0.5, pixmap, 176, 352, 0, 100, false));
  
  // Speaker controls
  fx_widgets.push_back(add_switch(fbox, n_speakers, 
				  302, 332, Switch::Green));
  fx_widgets.push_back(add_switch(fbox, n_speed, 
				  323, 356, Switch::BigRed));
  fx_widgets.push_back(add_knob(fbox, n_l_slow, 
				0, 1, 0.5, pixmap, 352, 352, 0, 10, true));
  fx_widgets.push_back(add_knob(fbox, n_l_fast, 
				0, 1, 0.5, pixmap, 396, 352, 0, 10, true));
  fx_widgets.push_back(add_knob(fbox, n_u_slow, 
				0, 1, 0.5, pixmap, 440, 352, 0, 10, true));
  fx_widgets.push_back(add_knob(fbox, n_u_fast, 
				0, 1, 0.5, pixmap, 484, 352, 0, 10, true));
  fx_widgets.push_back(add_knob(fbox, n_belt, 
				0, 1, 0.5, pixmap, 528, 352, 0, 100, false));
  fx_widgets.push_back(add_knob(fbox, n_spread, 
				0, 1, 0.5, pixmap, 572, 352, 0, 100, false));
  fx_widgets.push_back(add_switch(fbox, n_complex, 
				  443, 331, Switch::Mini));
  fx_widgets.push_back(add_switch(fbox, n_pedalspeed, 510, 331, Switch::Mini));

  // mode switcher 2
  Widget* eb2 = add_clickbox(vbox, 14, 53, 14, 44);
  eb2->signal_button_press_event().
    connect(sigc::hide(bind(bind(mem_fun(*this, &AZR3GUI::change_mode), 
				 ref(fbox)), true)));

  // vibrato controls
  add_switch(vbox, n_1_vibrato, 39, 17, Switch::Green);
  add_knob(vbox, n_1_vstrength, 0, 1, 0.5, voicepxm, 
	   88, 37, 0, 100, false);
  add_knob(vbox, n_1_vmix, 0, 1, 0.5, voicepxm, 176, 37, 0, 100, false);
  add_switch(vbox, n_2_vibrato, 302, 17, Switch::Green);
  add_knob(vbox, n_2_vstrength, 0, 1, 0.5, voicepxm, 
	   352, 37, 0, 100, false);
  add_knob(vbox, n_2_vmix, 0, 1, 0.5, voicepxm, 440, 37, 0, 100, false);
  
  pack_start(fbox);
    
  splitpoint_changed();
    
}
  

void AZR3GUI::set_control(uint32_t port, float value) {
  if (port < m_adj.size() && m_adj[port])
    m_adj[port]->set_value(value);
}
  
  

void AZR3GUI::add_program(unsigned char number, const char* name) {
  programs[number] = name;
  update_program_menu();
}



void AZR3GUI::remove_program(unsigned char number) {
  std::map<int, string>::iterator iter = programs.find(number);
  if (iter != programs.end()) {
    programs.erase(iter);
    update_program_menu();
  }
}
  
  

void AZR3GUI::set_program(unsigned char number) {
  std::map<int, string>::const_iterator iter = programs.find(number);
  ostringstream oss;
  oss<<"AZR-3 LV2 P"<<setw(2)<<setfill('0')<<int(number);
  tbox->set_string(0, oss.str());
  if (iter != programs.end())
    tbox->set_string(1, iter->second.substr(0, 23));
  current_program = number;
}



void AZR3GUI::clear_programs() {
  if (programs.size() > 0) {
    programs.clear();
    update_program_menu();
  }
}

  
void AZR3GUI::control_changed(uint32_t index, float new_value) {
  //m_write_function(m_controller, index, sizeof(float), &new_value);
  signal_set_control(index, new_value);
}

  
void AZR3GUI::program_changed(int program) {
  //m_program_function(m_controller, program);
  signal_set_program(program);
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
  if (splitswitch->get_adjustment().get_value() < 0.5)
    splitpoint_adj->set_value(0);
  else
    splitpoint_adj->set_value(splitkey / 128.0);
}


void AZR3GUI::set_back_pixmap(Widget* wdg, RefPtr<Pixmap> pm) {
  wdg->get_window()->set_back_pixmap(pm, false);
}


Knob* AZR3GUI::add_knob(Fixed& fbox, unsigned long port, 
			float min, float max, float value, 
			RefPtr<Pixmap>& bg, int xoffset, int yoffset,
			float dmin, float dmax, bool decimal) {
  Knob* knob = manage(new Knob(min, max, value, dmin, dmax, decimal));
  fbox.put(*knob, xoffset, yoffset);
  knob->modify_bg_pixmap(STATE_NORMAL, "<parent>");
  knob->modify_bg_pixmap(STATE_ACTIVE, "<parent>");
  knob->modify_bg_pixmap(STATE_PRELIGHT, "<parent>");
  knob->modify_bg_pixmap(STATE_SELECTED, "<parent>");
  knob->modify_bg_pixmap(STATE_INSENSITIVE, "<parent>");
  knob->get_adjustment().signal_value_changed().
    connect(compose(bind<0>(mem_fun(*this, &AZR3GUI::control_changed), port),
		    mem_fun(knob->get_adjustment(), &Adjustment::get_value)));
  assert(m_adj[port] == 0);
  m_adj[port] = &knob->get_adjustment();
  return knob;
}


Drawbar* AZR3GUI::add_drawbar(Fixed& fbox, unsigned long port, 
			      float min, float max, float value, 
			      RefPtr<Pixmap>& bg, int xoffset, int yoffset, 
			      Drawbar::Type type) {
  Drawbar* db = manage(new Drawbar(min, max, value, type));
  fbox.put(*db, xoffset, yoffset);
  db->modify_bg_pixmap(STATE_NORMAL, "<parent>");
  db->modify_bg_pixmap(STATE_ACTIVE, "<parent>");
  db->modify_bg_pixmap(STATE_PRELIGHT, "<parent>");
  db->modify_bg_pixmap(STATE_SELECTED, "<parent>");
  db->modify_bg_pixmap(STATE_INSENSITIVE, "<parent>");
  db->get_adjustment().signal_value_changed().
    connect(compose(bind<0>(mem_fun(*this, &AZR3GUI::control_changed), port),
		    mem_fun(db->get_adjustment(), &Adjustment::get_value)));
  assert(m_adj[port] == 0);
  m_adj[port] = &db->get_adjustment();
  
  return db;
}


Switch* AZR3GUI::add_switch(Fixed& fbox, long port,
			    int xoffset, int yoffset, Switch::Type type) {
  Switch* sw = manage(new Switch(type));
  fbox.put(*sw, xoffset, yoffset);
  if (port >= 0) {
    sw->get_adjustment().signal_value_changed().
      connect(compose(bind<0>(mem_fun(*this, &AZR3GUI::control_changed),port),
		      mem_fun(sw->get_adjustment(), &Adjustment::get_value)));
    if (port < m_adj.size()) {
      assert(m_adj[port] == 0);
      m_adj[port] = &sw->get_adjustment();
    }
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


Textbox* AZR3GUI::add_textbox(Fixed& fbox, RefPtr<Pixmap>& bg, 
			      int xoffset, int yoffset, int lines, 
			      int width, int height) {
  Textbox* db = manage(new Textbox(width, height, lines));
  fbox.put(*db, xoffset, yoffset);
  db->modify_bg_pixmap(STATE_NORMAL, "<parent>");
  db->modify_bg_pixmap(STATE_ACTIVE, "<parent>");
  db->modify_bg_pixmap(STATE_PRELIGHT, "<parent>");
  db->modify_bg_pixmap(STATE_SELECTED, "<parent>");
  db->modify_bg_pixmap(STATE_INSENSITIVE, "<parent>");
  return db;
}


bool AZR3GUI::change_mode(bool fx_mode, Fixed& fbox) {
  int x, y;
  if (fx_mode && !showing_fx_controls) {
    for (int i = 0; i < voice_widgets.size(); ++i) {
      voice_widgets[i]->translate_coordinates(fbox, 0, 0, x, y);
      fbox.move(*voice_widgets[i], x, y + 300);
    }
    for (int i = 0; i < fx_widgets.size(); ++i) {
      fx_widgets[i]->translate_coordinates(fbox, 0, 0, x, y);
      fbox.move(*fx_widgets[i], x, y - 300);
    }
  }
  else if (!fx_mode && showing_fx_controls) {
    for (int i = 0; i < fx_widgets.size(); ++i) {
      fx_widgets[i]->translate_coordinates(fbox, 0, 0, x, y);
      fbox.move(*fx_widgets[i], x, y + 300);
    }
    for (int i = 0; i < voice_widgets.size(); ++i) {
      voice_widgets[i]->translate_coordinates(fbox, 0, 0, x, y);
      fbox.move(*voice_widgets[i], x, y - 300);
    }
  }
  showing_fx_controls = fx_mode;
  return true;
}


void AZR3GUI::splitpoint_changed() {
  float value = m_adj[n_splitpoint]->get_value();
  int key = int(value * 128);
  if (key <= 0 || key >= 128) {
    tbox->set_string(2, "Keyboard split OFF");
    splitswitch->get_adjustment().set_value(0);
  }
  else {
    splitkey = key;
    tbox->set_string(2, string("Splitpoint: ") + note2str(key));
    splitswitch->get_adjustment().set_value(1);
  }
}


void AZR3GUI::update_program_menu() {
  program_menu->items().clear();
  std::map<int, string>::const_iterator iter;
  for (iter = programs.begin(); iter != programs.end(); ++iter) {
    ostringstream oss;
    oss<<setw(2)<<setfill('0')<<iter->first<<' '<<iter->second.substr(0, 23);
    MenuItem* item = manage(new MenuItem(oss.str()));
    item->signal_activate().
      connect(bind(mem_fun(*this, &AZR3GUI::program_changed), iter->first));
    program_menu->items().push_back(*item);
    item->show();
    item->get_child()->modify_fg(STATE_NORMAL, menu_fg);
  }
}


Menu* AZR3GUI::create_menu() {
  
  Color bg;
  bg.set_rgb(16000, 16000, 16000);
  menu_fg.set_rgb(65535, 65535, 50000);
  Menu* menu = manage(new Menu);
  
  program_menu = manage(new Menu);
  update_program_menu();
  
  MenuItem* program_item = manage(new MenuItem("Select program"));
  program_item->set_submenu(*program_menu);
  program_item->show();
  program_item->get_child()->modify_fg(STATE_NORMAL, menu_fg);
  
  MenuItem* save_item = manage(new MenuItem("Save program"));
  save_item->signal_activate().
    connect(mem_fun(*this, &AZR3GUI::save_program));
  save_item->show();
  save_item->get_child()->modify_fg(STATE_NORMAL, menu_fg);
  
  menu->items().push_back(*program_item);
  menu->items().push_back(*save_item);
  
  menu->modify_bg(STATE_NORMAL, bg);
  menu->modify_fg(STATE_NORMAL, menu_fg);
  program_menu->modify_bg(STATE_NORMAL, bg);
  program_menu->modify_fg(STATE_NORMAL, menu_fg);
  
  return menu;
}


bool AZR3GUI::popup_menu(GdkEventButton* event, Menu* menu) {
  menu->popup(event->button, event->time);
  return true;
}


void AZR3GUI::display_scroll(int line, GdkEventScroll* e) {
  if (line < 2) {
    std::map<int, string>::const_iterator iter = programs.find(current_program);
    if (iter == programs.end())
      iter = programs.begin();
    if (iter != programs.end()) {
      if (e->direction == GDK_SCROLL_UP) {
	++iter;
	iter = (iter == programs.end() ? programs.begin() : iter);
	//lv2.send_program(iter->first);
      }
      else if (e->direction == GDK_SCROLL_DOWN) {
	if (iter == programs.begin())
	  for (int i = 0; i < programs.size() - 1; ++i, ++iter);
	else
	  --iter;
	//lv2.send_program(iter->first);
      }
    }
  }
  else {
    int oldsplitkey = splitkey;
    if (splitswitch->get_adjustment().get_value() < 0.5)
      splitkey = 0;
    if (e->direction == GDK_SCROLL_UP)
      splitkey = (splitkey + 1) % 128;
    else if (e->direction == GDK_SCROLL_DOWN)
      splitkey = (splitkey - 1) % 128;
    while (splitkey < 0)
      splitkey += 128;
    if (oldsplitkey != splitkey)
      splitpoint_adj->set_value(splitkey / 128.0);
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
  spb.set_value(current_program);
  tbl.attach(lbl1, 0, 1, 0, 1);
  tbl.attach(lbl2, 0, 1, 1, 2);
  tbl.attach(ent, 1, 2, 0, 1);
  tbl.attach(spb, 1, 2, 1, 2);
  dlg.get_vbox()->pack_start(tbl);
  dlg.show_all();
  while (dlg.run() == RESPONSE_OK) {
    if (programs.find((unsigned char)adj.get_value()) != programs.end()) {
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


