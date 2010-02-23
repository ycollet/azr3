/****************************************************************************
    
    AZR-3 - An organ synth
    
    Copyright (C) 2006-2010 Lars Luthman <lars.luthman@gmail.com>
    
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

#include <string>

#include <jack/jack.h>
#include <gtkmm.h>
#include <pthread.h>
#include <semaphore.h>
#include <lash/lash.h>

#include "azr3.hpp"
#include "azr3gui.hpp"


struct Preset {
  Preset() : empty(true) { }
  std::string name;
  float values[63];
  bool empty;
};


struct Main {
  
  Main(int& argc, char**& argv);  
  
  void run();
  
  bool is_ok() const;
  
protected:
  
  void load_presets(char const* file);

  void write_presets(char const* file);

  void gui_changed_control(uint32_t index, float value);
  
  void gui_set_preset(unsigned char number);

  void gui_save_preset(unsigned char number, const std::string& name);

  bool engine_changed_control(uint32_t index, float value);

  void check_changes();

  int process(jack_nframes_t nframes);

  bool check_lash_events();

  bool init_lash(lash_args_t* lash_args, const std::string& jack_name);

  void auto_connect();

  static int static_process(jack_nframes_t frames, void* arg);


  jack_client_t* m_jack_client;
  jack_port_t* m_midi_port;
  jack_port_t* m_left_port;
  jack_port_t* m_right_port;
  AZR3* m_engine;
  AZR3GUI* m_gui;
  pthread_mutex_t m_engine_wlock;
  sem_t m_engine_changed;
  sem_t m_program_changed;
  float m_controls[63];
  unsigned char m_program;
  pthread_mutex_t m_gui_wlock;
  sem_t m_gui_changed;
  float m_gui_controls[63];
  Preset m_presets[128];
  lash_client_t* m_lash_client;

  bool m_ok;
  bool m_started_by_lashd;
  std::string m_auto_midi;
  std::string m_auto_audio;
  
  Gtk::Main* m_kit;
  Gtk::Window* m_win;

};
