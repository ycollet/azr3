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

#include <iostream>

#include "drawbar.hpp"
#include "dbblack.xpm"
#include "dbbrown.xpm"
#include "dbwhite.xpm"


using namespace Gtk;
using namespace Gdk;
using namespace Glib;
using namespace std;


Drawbar::Drawbar(float min, float max, float value, Type type) 
  : m_adj(value, min, max),
    m_type(type) {
  set_size_request(22, 150);
  add_events(EXPOSURE_MASK | BUTTON1_MOTION_MASK | 
             BUTTON_PRESS_MASK | SCROLL_MASK);
  m_adj.signal_value_changed().connect(mem_fun(*this, &Drawbar::queue_draw));
}
 
 
void Drawbar::set_value(float value) {
  m_adj.set_value(value);
}
  

Gtk::Adjustment& Drawbar::get_adjustment() {
  return m_adj;
}


void Drawbar::on_realize() {
  DrawingArea::on_realize();
  unsigned pixsize = 22 * 150;
  char bits[22 * 150];
  memset(bits, 0, sizeof(char) * pixsize);
  m_bitmap = Bitmap::create(bits, 22, 150);
  const char** xpm = dbblack;
  if (m_type == White)
    xpm = dbwhite;
  else if (m_type == Brown)
    xpm = dbbrown;
  m_pixmap = Pixmap::create_from_xpm(Colormap::get_system(), m_bitmap, xpm);
}


bool Drawbar::on_expose_event(GdkEventExpose* event) {
  RefPtr<Gdk::Window> win = get_window();
  RefPtr<GC> gc = GC::create(win);
  
  float value = (m_adj.get_value() - m_adj.get_lower()) / 
    (m_adj.get_upper() - m_adj.get_lower());
  value = value < 0 ? 0 : value;
  value = value > 1 ? 1 : value;
  value = 120 * value;
  
  win->draw_drawable(gc, m_pixmap, 0, 120 - int(value), 0, 0, 
                     22, 150 - (120 - int(value)));
  
  return true;
}


bool Drawbar::on_motion_notify_event(GdkEventMotion* event) {
  float value = m_value_offset + ((m_click_offset - event->y) / -120.0) * 
    (m_adj.get_upper() - m_adj.get_lower());
  value = value < m_adj.get_lower() ? m_adj.get_lower() : value;
  value = value > m_adj.get_upper() ? m_adj.get_upper() : value;
  m_adj.set_value(value);
  return true;
}


bool Drawbar::on_button_press_event(GdkEventButton* event) {
  m_click_offset = (int)event->y;
  m_value_offset = m_adj.get_value();
  return true;
}


bool Drawbar::on_scroll_event(GdkEventScroll* event) {
  if (event->direction == GDK_SCROLL_UP)
    m_adj.set_value(m_adj.get_value() - 
                    (m_adj.get_upper() - m_adj.get_lower()) / 30);
  else if (event->direction == GDK_SCROLL_DOWN)
    m_adj.set_value(m_adj.get_value() + 
                    (m_adj.get_upper() - m_adj.get_lower()) / 30);
  return true;
}
