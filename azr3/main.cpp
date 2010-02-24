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

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "main.hpp"
#include "options.hpp"

using namespace std;


Main::Main(int& argc, char**& argv) : m_ok(false) {
  
  /* this is a bit dumb, but the only way I know of to check whether we were
     started by lashd is to see if lash_extract_args() removes any arguments */
  int old_argc = argc;  
  lash_args_t* lash_args = lash_extract_args(&argc, &argv);
  m_started_by_lashd = (argc != old_argc);
  
  // parse all non-LASH arguments
  OptionParser op;
  bool help(false);
  bool version(false);
  unsigned preset_no(128);
  string jack_name("AZR-3");
  try {
    op.set_env_prefix("AZR3_JACK_")
      .add_bare("help", "h", help, 
		"Display this help text and exit.")
      .add_bare("version", "v", version, 
		"Display version information and exit.")
      .add("midi-input", "m", "PORT|CLIENT", m_auto_midi,
	   "When used, azr3 will try to connect its MIDI input\n"
	   "port to PORT (if it's a JACK port name) or the\n"
	   "first MIDI output port in CLIENT (if it's a JACK\n"
	   "client name).")
      .add("audio-output", "a", "PORT|CLIENT", m_auto_audio,
	   "When used, azr3 will try to connect its audio\n"
	   "output ports to PORT (if it's a JACK port name) or\n"
	   "the first two audio input ports in CLIENT (if it's\n"
	   "a JACK client name).")
      .add("preset", "p", "NUMBER", preset_no,
	   "Load the preset with the given number instead of\n"
	   "the first available one.")
      .add("jack-name", "j", "NAME", jack_name,
	   "Set the name of the JACK client. The default is\n"
	   "'AZR-3'. Note that JACK may change this name by\n"
	   "e.g. adding a number at the end if needed.")
      .parse_env()
      .parse(argc, argv);
  }
  catch (runtime_error& e) {
    cerr<<e.what()<<endl;
    return;
  }
  
  if (help || version) {
    cout<<argv[0]<<" version "<<PACKAGE_VERSION<<'\n'
	<<"This JACK version ported by Lars Luthman <lars.luthman@gmail.com>\n"
	<<"from the VST original by Rumpelrausch Täips\n\n";
    if (help) {
      cout<<"Program options:\n\n";
      op.print_help(cout);
      op.print_env_help(cout);
    }
    cout<<flush;
    return;
  }
    
  // initialise controls with default values
  float defaults[] = { 0.00, 0.20, 0.20, 0.00, 0.00, 0.75, 0.50, 0.60, 0.60, 
		       0.00, 0.22, 0.00, 1.00, 1.00, 0.00, 0.00, 0.00, 0.00,
		       0.00, 0.00, 0.00, 0.00, 0.30, 0.35, 0.00, 0.00, 0.00,
		       0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00,
		       0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.40,
		       0.00, 0.66, 0.00, 1.00, 0.00, 0.10, 0.65, 0.05, 0.78,
		       0.50, 0.50, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00 };
  memcpy(m_controls, defaults, 63 * sizeof(float));
  memcpy(m_gui_controls, defaults, 63 * sizeof(float));
  for (int i = 0; i < 128; ++i)
    memcpy(m_presets[i].values, defaults, 63 * sizeof(float));
  pthread_mutex_init(&m_engine_wlock, 0);
  pthread_mutex_init(&m_gui_wlock, 0);
  sem_init(&m_engine_changed, 0, 0);
  sem_init(&m_program_changed, 0, 0);
  sem_init(&m_gui_changed, 0, 0);
    
  // load presets
  load_presets(DATADIR "/presets");
  if (getenv("HOME")) {
    char buf[512];
    snprintf(buf, 512, "%s/.azr3_jack_presets", getenv("HOME"));
    load_presets(buf);
  }
    
  // initialise JACK client
  m_jack_client = jack_client_open(jack_name.c_str(), jack_options_t(0), 0);
  m_midi_port = jack_port_register(m_jack_client, "MIDI", 
				   JACK_DEFAULT_MIDI_TYPE, 
				   JackPortIsInput, 0);
  m_left_port = jack_port_register(m_jack_client, "Left", 
				   JACK_DEFAULT_AUDIO_TYPE,
				   JackPortIsOutput, 0);
  m_right_port = jack_port_register(m_jack_client, "Right", 
				    JACK_DEFAULT_AUDIO_TYPE,
				    JackPortIsOutput, 0);
  if (!(m_jack_client && m_midi_port && m_left_port && m_right_port)) {
    cerr<<"Could not initialise JACK client!"<<endl;
    return;
  }
  jack_set_process_callback(m_jack_client, &Main::static_process, this);
    
  // create the engine and connect the controls
  m_engine = new AZR3(jack_get_sample_rate(m_jack_client));
  for (uint32_t i = 0; i < 63; ++i)
    m_engine->connect_port(i, &m_controls[i]);
    
  // create GUI objects, initialise knobs and drawbars, connect signals
  m_kit = new Gtk::Main(argc, argv);
  m_win = new Gtk::Window;
  m_gui = new AZR3GUI;
  for (int i = 0; i < 128; ++i) {
    if (!m_presets[i].empty)
      m_gui->add_program(i, m_presets[i].name.c_str());
  }
  m_gui->signal_set_control.
    connect(sigc::mem_fun(*this, &Main::gui_changed_control));
  m_gui->signal_set_program.
    connect(sigc::mem_fun(*this, &Main::gui_set_preset));
  m_gui->signal_save_program.
    connect(sigc::mem_fun(*this, &Main::gui_save_preset));
  for (uint32_t i = 0; i < 63; ++i)
    m_gui->set_control(i, m_gui_controls[i]);
    
  // try to load the desired preset, if it doesn't work use the first one
  if (preset_no < 128 && !m_presets[preset_no].empty)
    gui_set_preset(preset_no);
  else {
    for (int i = 0; i < 128; ++i) {
      if (!m_presets[i].empty) {
	gui_set_preset(i);
	break;
      }
    }
  }
    
  // initialise LASH
  if (!init_lash(lash_args, jack_get_client_name(m_jack_client)))
    return;
  
  m_win->set_title("AZR-3");
  m_win->set_resizable(false);
  m_win->add(*m_gui);
  m_win->show_all();
    
  m_ok = true;
}
  
  
void Main::run() {
  m_engine->activate();
  jack_activate(m_jack_client);

  // auto-connect JACK ports if desired
  if (!m_started_by_lashd)
    auto_connect();

  Glib::signal_timeout().
    connect(sigc::bind_return(sigc::mem_fun(*this, &Main::check_changes), 
			      true), 10);
  Glib::signal_timeout().
    connect(sigc::mem_fun(*this, &Main::check_lash_events), 500);
  m_kit->run(*m_win);
  jack_deactivate(m_jack_client);
  m_engine->deactivate();
}
  
  
bool Main::is_ok() const {
  return m_ok;
}

  
void Main::load_presets(char const* file) {
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
      m_presets[number].values[i] = value;
      fin>>ws;
    }
    getline(fin, m_presets[number].name);
    fin>>ws;
    m_presets[number].empty = false;
    cout<<"Loaded program "<<number<<": "<<m_presets[number].name<<endl;
  }
}


void Main::write_presets(char const* file) {
  ofstream fout(file);
  for (unsigned char i = 0; i < 128; ++i) {
    if (!m_presets[i].empty) {
      fout<<int(i);
      for (uint32_t p = 0; p < 63; ++p)
	fout<<" "<<m_presets[i].values[p];
      fout<<" "<<m_presets[i].name<<endl;
    }
  }
}


void Main::gui_changed_control(uint32_t index, float value) {
  pthread_mutex_lock(&m_gui_wlock);
  m_gui_controls[index] = value;
  pthread_mutex_unlock(&m_gui_wlock);
  sem_post(&m_gui_changed);
}
  
  
void Main::gui_set_preset(unsigned char number) {
  if (number < 128) {
    pthread_mutex_lock(&m_gui_wlock);
    for (int i = 0; i < 63; ++i)
      m_gui_controls[i] = m_presets[number].values[i];
    m_program = number;
    pthread_mutex_unlock(&m_gui_wlock);
    sem_post(&m_gui_changed);
    for (int i = 0; i < 63; ++i)
      m_gui->set_control(i, m_presets[number].values[i]);
    m_gui->set_program(number);
  }
}


void Main::gui_save_preset(unsigned char number, const string& name) {
  if (number < 128) {
    m_presets[number].empty = false;
    memcpy(&m_presets[number].values[0], m_gui_controls, sizeof(float) * 63);
    m_presets[number].name = name;
    m_gui->add_program(number, name.c_str());
    m_gui->set_program(number);
    m_program = number;
    if (getenv("HOME")) {
      char buf[512];
      snprintf(buf, 512, "%s/.azr3_jack_presets", getenv("HOME"));
      write_presets(buf);
    }
  }
}
  

bool Main::engine_changed_control(uint32_t index, float value) {
  if (pthread_mutex_trylock(&m_engine_wlock))
    return false;
  m_controls[index] = value;
  pthread_mutex_unlock(&m_engine_wlock);
  sem_post(&m_engine_changed);
  return true;
}
  

void Main::check_changes() {
  static float tmp[63];
  if (!sem_trywait(&m_engine_changed)) {
    while (!sem_trywait(&m_engine_changed));
    pthread_mutex_lock(&m_engine_wlock);
    memcpy(tmp, m_controls, sizeof(float) * 63);
    pthread_mutex_unlock(&m_engine_wlock);
    for (uint32_t i = 0; i < 63; ++i) {
      if (tmp[i] != m_gui_controls[i]) {
	m_gui_controls[i] = tmp[i];
	m_gui->set_control(i, tmp[i]);
      }
    }
  }
  if (!sem_trywait(&m_program_changed)) {
    while (!sem_trywait(&m_program_changed));
    gui_set_preset(m_program);
  }
}


int Main::process(jack_nframes_t nframes) {
    
  // get control changes from the GUI thread
  int sem_val = 0;
  sem_getvalue(&m_gui_changed, &sem_val);
  if (sem_val) {
    if (!pthread_mutex_trylock(&m_gui_wlock)) {
      while (!sem_trywait(&m_gui_changed));
      memcpy(m_controls, m_gui_controls, 63 * sizeof(float));
      pthread_mutex_unlock(&m_gui_wlock);
    }
  }
    
  m_engine->connect_port(63, jack_port_get_buffer(m_midi_port, nframes));
  m_engine->connect_port(64, jack_port_get_buffer(m_left_port, nframes));
  m_engine->connect_port(65, jack_port_get_buffer(m_right_port, nframes));
    
  // XXX It isn't really safe to write automated changes here since the GUI
  //     may be reading from the control values - the mutex isn't locked
  m_engine->run(nframes);
    
  if (m_engine->controls_has_changed())
    sem_post(&m_engine_changed);
  unsigned char prog = m_engine->received_program_change();
  if (prog != 255) {
    m_program = prog;
    sem_post(&m_program_changed);
  }
    
  return 0;
}


bool Main::check_lash_events() {
  lash_event_t* event;
  bool go_on = true;
  while ((event = lash_get_event(m_lash_client))) {
      
    // save
    if (lash_event_get_type(event) == LASH_Save_File) {
      cerr<<"Received LASH Save command"<<endl;
      string dir(lash_event_get_string(event));
      ofstream fout((dir + "/state").c_str());
      fout<<int(m_program);
      for (uint32_t i = 0; i < 63; ++i)
	fout<<" "<<m_gui_controls[i];
      fout<<endl;
      write_presets((dir + "/presets").c_str());
      lash_send_event(m_lash_client, 
		      lash_event_new_with_type(LASH_Save_File));
    }
      
    // restore
    else if (lash_event_get_type(event) == LASH_Restore_File) {
      cerr<<"Received LASH Restore command"<<endl;
      string dir(lash_event_get_string(event));
      for (unsigned char i = 0; i < 128; ++i)
	m_presets[i].empty = true;
      load_presets((dir + "/presets").c_str());
      m_gui->clear_programs();
      for (unsigned char i = 0; i < 128; ++i) {
	if (!m_presets[i].empty)
	  m_gui->add_program(i, m_presets[i].name.c_str());
      }
      ifstream fin((dir + "/state").c_str());
      int prog;
      fin>>prog;
      m_gui->set_program(prog);
      for (uint32_t p = 0; p < 63; ++p) {
	float tmp;
	fin>>tmp;
	m_gui->set_control(p, tmp);
      }
      lash_send_event(m_lash_client, 
		      lash_event_new_with_type(LASH_Restore_File));
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


bool Main::init_lash(lash_args_t* lash_args, const std::string& jack_name) {
  
  m_lash_client = lash_init(lash_args, "AZR-3", 
			    LASH_Config_File, LASH_PROTOCOL(2, 0));
  if (m_lash_client) {
    lash_event_t* event = lash_event_new_with_type(LASH_Client_Name);
    lash_event_set_string(event, "AZR-3");
    lash_send_event(m_lash_client, event);      
    lash_jack_client_name(m_lash_client, jack_name.c_str());
    Glib::signal_timeout().
      connect(sigc::mem_fun(*this, &Main::check_lash_events), 500);
  }
  else
    cerr<<"Could not initialise LASH!"<<endl;
  return (m_lash_client != 0);
}
  
  
int Main::static_process(jack_nframes_t frames, void* arg) {
  return static_cast<Main*>(arg)->process(frames);
}


void Main::auto_connect() {

  const char* env;
  const char** port_list;
  const char* name = jack_get_client_name(m_jack_client);

  // MIDI input
  if (m_auto_midi != "") {

    // if it's a client, connect to the first MIDI output port
    if (m_auto_midi.find(':') == string::npos &&
	(port_list = jack_get_ports(m_jack_client, (m_auto_midi + ":*").c_str(),
				    JACK_DEFAULT_MIDI_TYPE, 
				    JackPortIsOutput)) && port_list[0]) {
      jack_connect(m_jack_client, port_list[0], jack_port_name(m_midi_port));
      free(port_list);
    }

    // if not, connect to that port
    else
      jack_connect(m_jack_client, m_auto_midi.c_str(), 
		   jack_port_name(m_midi_port));
  }

  // audio output
  if (m_auto_audio != "") {
    
    // if it's a client, connect individual ports
    if (m_auto_audio.find(':') == string::npos &&
	(port_list = jack_get_ports(m_jack_client, 
				    (m_auto_audio + ":*").c_str(),
				    JACK_DEFAULT_AUDIO_TYPE, 
				    JackPortIsInput)) && port_list[0]) {
      if (port_list[0])
	jack_connect(m_jack_client, jack_port_name(m_left_port), port_list[0]);
      if (port_list[1])
	jack_connect(m_jack_client, jack_port_name(m_right_port), port_list[1]);
      free(port_list);
    }

    // if not, connect all our ports to that single port
    else {
      jack_connect(m_jack_client, 
		   jack_port_name(m_left_port), m_auto_audio.c_str());
      jack_connect(m_jack_client, 
		   jack_port_name(m_right_port), m_auto_audio.c_str());
    }
  }
}



int main(int argc, char** argv) {
  Main m(argc, argv);
  if (m.is_ok())
    m.run();
  return 0;
}
