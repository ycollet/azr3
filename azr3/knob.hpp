/****************************************************************************
    
    AZR-3 - An LV2 synth plugin
    
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

#ifndef KNOB_HPP
#define KNOB_HPP

#include <gtkmm.h>


class Knob : public Gtk::DrawingArea {
public:
  
  Knob(float min, float max, float value, 
       float dmin, float dmax, bool decimal);
  
  void set_value(float value);
  
  Gtk::Adjustment& get_adjustment();
  
protected:
  
  void on_realize();
  bool on_expose_event(GdkEventExpose* event);
  bool on_motion_notify_event(GdkEventMotion* event);
  bool on_button_press_event(GdkEventButton* event);
  bool on_scroll_event(GdkEventScroll* event);
  
  void draw_digits(Glib::RefPtr<Gdk::Window>& win, Glib::RefPtr<Gdk::GC>& gc);
  void draw_digit(Glib::RefPtr<Gdk::Window>& win, Glib::RefPtr<Gdk::GC>& gc,
                  int xoffset, int digit);
  
  Gtk::Adjustment m_adj;
  Glib::RefPtr<Gdk::Pixmap> m_pixmap;
  Glib::RefPtr<Gdk::Bitmap> m_bitmap;
  Glib::RefPtr<Gdk::Pixmap> m_digpix;
  Glib::RefPtr<Gdk::Bitmap> m_digbit;
  int m_click_offset;
  float m_value_offset;
  float m_dmin;
  float m_dmax;
  bool m_decimal;
};


#endif
