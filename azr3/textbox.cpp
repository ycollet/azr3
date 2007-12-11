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

#include "textbox.hpp"


using namespace std;
using namespace Gtk;
using namespace Gdk;
using namespace Glib;
using namespace Pango;


Textbox::Textbox(int width, int height, int lines) 
  : m_width(width),
    m_height(height),
    m_strings(lines) {
  
  FontDescription fd("courier,monaco,monospace bold");
  pango_font_description_set_absolute_size(fd.gobj(), 10 * PANGO_SCALE);
  
  for (int i = 0; i < lines; ++i) {
    m_strings[i] = Pango::Layout::create(get_pango_context());
    m_strings[i]->set_font_description(fd);
  }
  
  RefPtr<Colormap> cmap = Colormap::get_system();
  m_color.set_rgb(65535, 65535, 50000);
  cmap->alloc_color(m_color);
  
  set_string(0, "AZR-3 JACK");
  
  set_size_request(width, height);
}
  

void Textbox::set_string(unsigned line, const std::string& str) {
  if (line >= 0 && line < m_strings.size()) {
    m_strings[line]->set_text(str);
    queue_draw();
  }
}
  

bool Textbox::on_expose_event(GdkEventExpose* event) {
  RefPtr<Gdk::Window> win = get_window();
  RefPtr<GC> gc = GC::create(win);
  gc->set_foreground(m_color);
  for (unsigned i = 0; i < m_strings.size(); ++i) {
    int y = i * (m_height / m_strings.size());
    win->draw_layout(gc, 0, y, m_strings[i]);
  }
  return true;
}


bool Textbox::on_scroll_event(GdkEventScroll* event) {
  int line = int(event->y) / (m_height / m_strings.size());
  signal_scroll_display(line, event);
  return true;
}
