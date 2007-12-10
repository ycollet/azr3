#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include <jack/jack.h>
#include <gtkmm.h>
#include <pthread.h>
#include <semaphore.h>
#include <lash/lash.h>

#include "azr3.hpp"
#include "azr3_gtk.hpp"


using namespace std;


struct Preset {
  Preset() : empty(true) { }
  string name;
  float values[63];
  bool empty;
};


struct State {
  jack_client_t* jack_client;
  jack_port_t* midi_port;
  jack_port_t* left_port;
  jack_port_t* right_port;
  AZR3* engine;
  pthread_mutex_t engine_wlock;
  sem_t engine_changed;
  sem_t program_changed;
  float controls[63];
  unsigned char program;
  pthread_mutex_t gui_wlock;
  sem_t gui_changed;
  float gui_controls[63];
  Preset presets[128];
  lash_client_t* lash_client;
};


void load_presets(char const* file, State* s) {
  ifstream fin(file);
  fin>>ws;
  while (fin.good()) {
    int number;
    fin>>number;
    if (number < 0 && number > 127) {
      cerr<<"Invalid program number: "<<number<<endl
	  <<"Skipping the rest of "<<file<<endl;
      break;
    }
    for (int i = 0; i < 63; ++i) {
      float value;
      fin>>value;
      s->presets[number].values[i] = value;
      fin>>ws;
    }
    getline(fin, s->presets[number].name);
    fin>>ws;
    s->presets[number].empty = false;
    cout<<"Loaded program "<<number<<": "<<s->presets[number].name<<endl;
  }
}


void write_presets(char const* file, State* s) {
  ofstream fout(file);
  for (unsigned char i = 0; i < 128; ++i) {
    if (!s->presets[i].empty) {
      fout<<int(i);
      for (uint32_t p = 0; p < 63; ++p)
	fout<<" "<<s->presets[i].values[p];
      fout<<" "<<s->presets[i].name<<endl;
    }
  }
}


void gui_changed_control(uint32_t index, float value, State* s) {
  pthread_mutex_lock(&s->gui_wlock);
  s->gui_controls[index] = value;
  pthread_mutex_unlock(&s->gui_wlock);
  sem_post(&s->gui_changed);
}


void gui_set_preset(unsigned char number, State* s, AZR3GUI* gui) {
  if (number < 128) {
    pthread_mutex_lock(&s->gui_wlock);
    for (int i = 0; i < 63; ++i)
      s->gui_controls[i] = s->presets[number].values[i];
    pthread_mutex_unlock(&s->gui_wlock);
    sem_post(&s->gui_changed);
    for (int i = 0; i < 63; ++i)
      gui->set_control(i, s->presets[number].values[i]);
    gui->set_program(number);
  }
}


void gui_save_preset(unsigned char number, const string& name, 
		     State* s, AZR3GUI* gui) {
  if (number < 128) {
    s->presets[number].empty = false;
    memcpy(&s->presets[number].values[0], s->gui_controls, sizeof(float) * 63);
    s->presets[number].name = name;
    gui->add_program(number, name.c_str());
    gui->set_program(number);
    if (getenv("HOME")) {
      char buf[512];
      snprintf(buf, 512, "%s/.azr3_jack_presets", getenv("HOME"));
      write_presets(buf, s);
    }
  }
}


bool engine_changed_control(uint32_t index, float value, State* s) {
  if (pthread_mutex_trylock(&s->engine_wlock))
    return false;
  s->controls[index] = value;
  pthread_mutex_unlock(&s->engine_wlock);
  sem_post(&s->engine_changed);
  return true;
}


void check_changes(AZR3GUI* gui, State* s) {
  static float tmp[63];
  if (!sem_trywait(&s->engine_changed)) {
    cerr<<"Yup"<<endl;
    while (!sem_trywait(&s->engine_changed));
    pthread_mutex_lock(&s->engine_wlock);
    memcpy(tmp, s->controls, sizeof(float) * 63);
    pthread_mutex_unlock(&s->engine_wlock);
    for (uint32_t i = 0; i < 63; ++i) {
      if (tmp[i] != s->gui_controls[i]) {
	s->gui_controls[i] = tmp[i];
	gui->set_control(i, tmp[i]);
      }
    }
  }
  if (!sem_trywait(&s->program_changed)) {
    while (!sem_trywait(&s->program_changed));
    gui_set_preset(s->program, s, gui);
  }
}


int process(jack_nframes_t nframes, void* arg) {
  
  State* s = static_cast<State*>(arg);
  
  // get control changes from the GUI thread
  int sem_val = 0;
  sem_getvalue(&s->gui_changed, &sem_val);
  if (sem_val) {
    if (!pthread_mutex_trylock(&s->gui_wlock)) {
      while (!sem_trywait(&s->gui_changed));
      memcpy(s->controls, s->gui_controls, 63 * sizeof(float));
      pthread_mutex_unlock(&s->gui_wlock);
    }
  }
  
  s->engine->connect_port(63, jack_port_get_buffer(s->midi_port, nframes));
  s->engine->connect_port(64, jack_port_get_buffer(s->left_port, nframes));
  s->engine->connect_port(65, jack_port_get_buffer(s->right_port, nframes));
  
  // XXX It isn't really safe to write automated changes here since the GUI
  //     may be reading from the control values - the mutex isn't locked
  s->engine->run(nframes);
  
  if (s->engine->controls_has_changed())
    sem_post(&s->engine_changed);
  unsigned char prog = s->engine->received_program_change();
  if (prog != 255) {
    s->program  = prog;
    sem_post(&s->program_changed);
  }
  
  return 0;
}


bool check_lash_events(State* s, AZR3GUI* gui) {
  lash_event_t* event;
  bool go_on = true;
  while ((event = lash_get_event(s->lash_client))) {
    
    // save
    if (lash_event_get_type(event) == LASH_Save_File) {
      cerr<<"Received LASH Save command"<<endl;
      string dir(lash_event_get_string(event));
      ofstream fout((dir + "/state").c_str());
      fout<<int(s->program);
      for (uint32_t i = 0; i < 63; ++i)
	fout<<" "<<s->gui_controls[i];
      fout<<endl;
      write_presets((dir + "/presets").c_str(), s);
      lash_send_event(s->lash_client, lash_event_new_with_type(LASH_Save_File));
    }
    
    // restore
    else if (lash_event_get_type(event) == LASH_Restore_File) {
      cerr<<"Received LASH Restore command"<<endl;
      string dir(lash_event_get_string(event));
      for (unsigned char i = 0; i < 128; ++i)
	s->presets[i].empty = true;
      load_presets((dir + "/presets").c_str(), s);
      gui->clear_programs();
      for (unsigned char i = 0; i < 128; ++i) {
	if (!s->presets[i].empty)
	  gui->add_program(i, s->presets[i].name.c_str());
      }
      ifstream fin((dir + "/state").c_str());
      int prog;
      fin>>prog;
      gui->set_program(prog);
      for (uint32_t p = 0; p < 63; ++p) {
	float tmp;
	fin>>tmp;
	gui->set_control(p, tmp);
      }
      lash_send_event(s->lash_client, lash_event_new_with_type(LASH_Restore_File));
    }
    
    // quit
    else if (lash_event_get_type(event) == LASH_Quit) {
      cerr<<"Received LASH Quit command"<<endl;
      Gtk::Main::instance()->quit();
      go_on = false;
    }
    
    lash_event_destroy(event);
  }
  return go_on;
}


bool init_lash(int& argc, char**& argv, const std::string& jack_name, 
	       State* s, AZR3GUI* gui) {
  s->lash_client = lash_init(lash_extract_args(&argc, &argv), "AZR-3", 
			     LASH_Config_File, LASH_PROTOCOL(2, 0));
  if (s->lash_client) {
    lash_event_t* event = lash_event_new_with_type(LASH_Client_Name);
    lash_event_set_string(event, "AZR-3");
    lash_send_event(s->lash_client, event);      
    lash_jack_client_name(s->lash_client, jack_name.c_str());
    Glib::signal_timeout().
      connect(sigc::bind(sigc::bind(&check_lash_events, gui), s), 500);
  }
  else
    cerr<<"Could not initialise LASH!"<<endl;
  return (s->lash_client != 0);
}




int main(int argc, char** argv) {
  
  State s;
  
  // initialise controls with default values
  float defaults[] = { 0.00, 0.20, 0.20, 0.00, 0.00, 0.75, 0.50, 0.60, 0.60, 
		       0.00, 0.22, 0.00, 1.00, 1.00, 0.00, 0.00, 0.00, 0.00,
		       0.00, 0.00, 0.00, 0.00, 0.30, 0.35, 0.00, 0.00, 0.00,
		       0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00,
		       0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.40,
		       0.00, 0.66, 0.00, 1.00, 0.00, 0.10, 0.65, 0.05, 0.78,
		       0.50, 0.50, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00 };
  memcpy(s.controls, defaults, 63 * sizeof(float));
  memcpy(s.gui_controls, defaults, 63 * sizeof(float));
  for (int i = 0; i < 128; ++i)
    memcpy(s.presets[i].values, defaults, 63 * sizeof(float));
  pthread_mutex_init(&s.engine_wlock, 0);
  pthread_mutex_init(&s.gui_wlock, 0);
  sem_init(&s.engine_changed, 0, 0);
  sem_init(&s.program_changed, 0, 0);
  sem_init(&s.gui_changed, 0, 0);
  
  // load presets
  load_presets(DATADIR "/presets", &s);
  if (getenv("HOME")) {
    char buf[512];
    snprintf(buf, 512, "%s/.azr3_jack_presets", getenv("HOME"));
    load_presets(buf, &s);
  }
  
  // initialise JACK client
  s.jack_client = jack_client_open("AZR-3", jack_options_t(0), 0);
  s.midi_port = jack_port_register(s.jack_client, "MIDI", 
				   JACK_DEFAULT_MIDI_TYPE, 
				   JackPortIsInput, 0);
  s.left_port = jack_port_register(s.jack_client, "Left", 
				   JACK_DEFAULT_AUDIO_TYPE,
				   JackPortIsOutput, 0);
  s.right_port = jack_port_register(s.jack_client, "Right", 
				    JACK_DEFAULT_AUDIO_TYPE,
				    JackPortIsOutput, 0);
  if (!(s.jack_client && s.midi_port && s.left_port && s.right_port)) {
    cerr<<"Could not initialise JACK client!"<<endl;
    return -1;
  }
  jack_set_process_callback(s.jack_client, &process, &s);
  
  // create the engine and connect the controls
  s.engine = new AZR3(jack_get_sample_rate(s.jack_client));
  for (uint32_t i = 0; i < 63; ++i)
    s.engine->connect_port(i, &s.controls[i]);
  
  // create GUI objects, initialise knobs and drawbars, connect signals
  Gtk::Main kit(argc, argv);
  Gtk::Window win;
  AZR3GUI gui;
  for (int i = 0; i < 128; ++i) {
    if (!s.presets[i].empty)
      gui.add_program(i, s.presets[i].name.c_str());
  }
  gui.signal_set_control.connect(sigc::bind(&gui_changed_control, &s));
  gui.signal_set_program.
    connect(sigc::bind(sigc::bind(&gui_set_preset, &gui), &s));
  gui.signal_save_program.
    connect(sigc::bind(sigc::bind(&gui_save_preset, &gui), &s));
  for (uint32_t i = 0; i < 63; ++i)
    gui.set_control(i, s.gui_controls[i]);

  // find and use the first preset
  for (int i = 0; i < 128; ++i) {
    if (!s.presets[i].empty) {
      gui_set_preset(i, &s, &gui);
      break;
    }
  }

  // initialise LASH
  if (!init_lash(argc, argv, jack_get_client_name(s.jack_client), &s, &gui))
    return -1;
  
  win.set_title("AZR-3");
  win.set_resizable(false);
  win.add(gui);
  win.show_all();
  
  // run
  s.engine->activate();
  jack_activate(s.jack_client);
  Glib::signal_timeout().
    connect(sigc::bind_return(sigc::bind(sigc::bind(&check_changes, &s), 
					 &gui), true), 10);
  Glib::signal_timeout().
    connect(sigc::bind(sigc::bind(&check_lash_events, &gui), &s), 500);
  kit.run(win);
  jack_deactivate(s.jack_client);
  s.engine->deactivate();
  
  return 0;
}
