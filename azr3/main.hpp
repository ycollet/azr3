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

  bool init_lash(int& argc, char**& argv,
		 const std::string& jack_name, bool& started_by_lashd);

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
  
  Gtk::Main* m_kit;
  Gtk::Window* m_win;

};
