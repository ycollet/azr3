/****************************************************************************
  
  newjack.hpp - compatibility header for JACK APIs post 0.107
  
  Copyright (C) 2008 Lars Luthman <lars.luthman@gmail.com>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2 as published
  by the Free Software Foundation.
    
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307 USA

****************************************************************************/

#ifndef NEWJACK_HPP
#define NEWJACK_HPP

#include <jack/midiport.h>


inline jack_nframes_t jack_midi_get_event_count(void* port_buffer, 
						jack_nframes_t nframes) {
  return jack_midi_get_event_count(port_buffer);
}


inline int jack_midi_event_get(jack_midi_event_t* event, 
			       void* port_buffer,
			       jack_nframes_t event_index, 
			       jack_nframes_t nframes) {
  return jack_midi_event_get(event, port_buffer, event_index);
}


#endif
