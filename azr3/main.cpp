#include <cstring>
#include <iostream>

#include <jack/jack.h>
#include <gtkmm.h>

#include "azr3.hpp"
#include "azr3_gtk.hpp"


using namespace std;


struct State {
  jack_client_t* jack_client;
  jack_port_t* midi_port;
  jack_port_t* left_port;
  jack_port_t* right_port;
  AZR3* engine;
  float controls[63];
  float gui_controls[63];
};


int process(jack_nframes_t nframes, void* arg) {
  
  State* s = static_cast<State*>(arg);
  
  s->engine->connect_port(63, jack_port_get_buffer(s->midi_port, nframes));
  s->engine->connect_port(64, jack_port_get_buffer(s->left_port, nframes));
  s->engine->connect_port(65, jack_port_get_buffer(s->right_port, nframes));
  
  cout<<((float*)jack_port_get_buffer(s->left_port, nframes))[0]<<endl;
  
  s->engine->run(nframes);
  
  return 0;
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
  
  // create GUI objects, initialise knobs and drawbars
  Gtk::Main kit(argc, argv);
  Gtk::Window win;
  AZR3GUI gui;
  for (uint32_t i = 0; i < 63; ++i)
    gui.set_control(i, s.gui_controls[i]);
  win.set_title("AZR-3");
  win.add(gui);
  win.show_all();
  
  // run
  jack_activate(s.jack_client);
  kit.run(win);
  jack_deactivate(s.jack_client);
  
  return 0;
}
