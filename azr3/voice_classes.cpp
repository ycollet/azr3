/****************************************************************************
    
    AZR-3 - An organ synth
    
    Copyright (C) 2006-2007 Lars Luthman <lars.luthman@gmail.com>
    
    based on source code from the VST plugin AZR-3, (C) 2006 Philipp Mott
    (well, almost all of it is his code)
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as 
    published by the Free Software Foundation.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA

****************************************************************************/

/*
  The voice producing machine, diverted into two blocks:
  
  VOICES actually calculate a single voice sound. They
  do basic voice handling like ADSR, pitch and the unevitable
  click and percussion effects.

  A NOTEMASTER instantiates as many voice class objects as needed
  for the current instrument. The numbers are tunable in "Globals.h",
  definition "NUMOFVOICES". It handles all incoming events like note
  on and off, all notes off, pedal, pitch bend and so on. The events
  will then be assigned to the corresponding voices. The notemaster
  assigns a specific wavetable to each voice.	Only active voices
  are handled by the notemaster - that's the main reason why the CPU
  meter goes up when you hold more notes. The positive effect is: The
  meter goes down if you use less voices...
*/
#include "voice_classes.hpp"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits>
#ifdef WIN32
#include <wtypes.h>
#endif


#include <iostream>


using namespace std;


void voice::reset() {
  actual_note = -1;
  next_note = -1;
  perc_next_note = -1;
  perc_ok = false;
  vca_phase = VP_IDLE;
  output = 0;
  VCA = 0;
  perc = 0;
  perc_vca = 0;
  percmultiplier = 0;
  adsr_decay = 0;
  click = 0;
  status = VS_IDLE;
  pedal = false;
  pitch = 1;
  phase = phaseinc = 0;
  sustain = 0;
}


void voice::suspend() {
  actual_note = -1;
  next_note = -1;
  perc_next_note = -1;
  vca_phase = VP_IDLE;
  output = 0;
  VCA = 0;
  perc_vca = 0;
  status = VS_IDLE;
  pitch = 1;
  phase = phaseinc = 0;
}


void voice::resume() {

}


voice::voice() {
  midi_scaler = (1. / 127.);

  long i;

  double k = 1.059463094359;	// 12th root of 2
  double a = 6.875;	// a
  a *= k;	// b
  a *= k;	// bb
  a *= k;	// c, frequency of midi note 0
  for (i = 0; i  <  128; i++) {
    freqtab[i] = (float)a;
    a *= k;
  }
  samplecount1 = samplecount2 = 0;
  sustain = 0;
  perc_ok = false;
}


void voice::set_samplerate(float samplerate) {
  this->samplerate = samplerate;
  clicklp.set_samplerate(samplerate);
}

void voice::voicecalc() {
  /*
    This "scaler" stuff is important to keep timing independant from
    the actual samplerate.
  */
  float	scaler;
  scaler = samplerate / 44100;

  clickattack = .004f / scaler;
  adsr_attack = .05f / scaler;

  adsr_decay = 0 / scaler;
  if (sustain > 0)
    adsr_release = (.0001f + .0005f * (1 - sustain)) / scaler;
  else
    adsr_release = pow(.90197, 1.0 / scaler);

  adsr_fast_release = .03f / scaler;
	
  perc_decay = perc_fade * .00035f / scaler;

  perc_decay *= 3;

  samplerate_scaler = (float)((double)my_size / (double)samplerate);
  phaseinc = (float)freqtab[actual_note] * samplerate_scaler * pitch;
  if (perc_phase == 0) {
    // fast sine calculation, precise enough for our percussion effect
    float	fact = percmultiplier * freqtab[actual_note];
    while(fact>3800)
      fact *= .5f;
    a = 2 * sinf(3.14159265358979f * pitch * fact / samplerate);
    s0 = .5f;
    s1 = 0;
  }
}


float voice::clock() {
  if (status == VS_IDLE||actual_note < 0)	// nothing to do...
    return(0);

  output = 0;
  /*
    This is the part where we read a value from the assigned wavetable.
    We use a very simple interpolation to determine the actual sample
    value. Since we use almost pure sine waves we don't have to take care
    about anti-aliasing here. The few aliasing effects we receive sound just
    like those real hardware tonewheels...
  */
  iphase = int(phase);
  fract = phase-iphase;
  y0 = my_table[iphase];
  y1 = my_table[iphase+1];
  output = (y0+fract * (y1-y0)) * VCA;

  phase  +=  phaseinc;

  /*
    No, we don't use the bit mask stuff as mentioned in the SDK.
    It's _not_ slower this way, and we can have random wavetable sizes.
  */
  if (phase>my_size)
    phase -= my_size;

  samplecount1++;
  
  // do envelope stuff every 5 samples
  if (samplecount1 > 5) {
    samplecount1 = 0;
      
    // attack - gradually increase the envelope to 1
    if (vca_phase == VP_A) {
      if (click <= 0) {
	VCA += adsr_attack;
      }

      if (VCA > 1) {
	VCA = 1;
	vca_phase = VP_D;
      }
    }
    
    // decay - just skip to sustain immediately
    else if (vca_phase == VP_D) {
      vca_phase = VP_S;
    }
    
    // sustain - just wait for the key release
    else if (vca_phase == VP_S) {
      // output *= adsr_sustain;
    }
    
    // release - gradually decrease the envelope to 0
    else if (vca_phase == VP_R) {
      
      // turn off the percussion sound
      if (perc > 0) {
	if (sustain == 0)
	  perc_phase = 3;
      }
      
      // decrease the envelope
      if (sustain > 0)
	VCA -= adsr_release;
      else
	VCA *= adsr_release;
      
      // go to idle state if we have almost reached 0
      if (VCA <= 0.00001f) {
	VCA = 0;
	actual_note = -1;
	phase = 0;
	vca_phase = VP_IDLE;
	output = 0;
	status = VS_IDLE;
      }
    }
    
    // fast release - turn everything off NOW
    else if (vca_phase == VP_FR) {
      
      VCA -= adsr_fast_release;
      
      if (VCA <= 0) {
	VCA = 0;
	actual_note = -1;
	if (next_note >= 0) {
	  actual_note = next_note;
	  next_note = -1;
	  vca_phase = VP_A;
	  phase = 0;
	  perc_phase = 0;
	  this->voicecalc();
		  
	  // retrigger percussion
	  if (perc_ok && perc>0 && percmultiplier>0) {
	    perc_phase = 1;
	    perc_vca = 0;
	  }
	}
	else
	  status = VS_IDLE;
      }
    }
    
  }
  
  // if we're in the attack state and click is on, generate a click
  if (vca_phase == VP_A && click > 0) {
    static unsigned long randSeed = 22222;
    float rand = 0;
    float mattack = 0;
    if (mattack < 1)
      mattack = VCA * 8;
    if (mattack>1)
      mattack = 1;
    randSeed = (randSeed * 196314165) + 907633515;
    rand = (float)randSeed / std::numeric_limits<unsigned long>::max();
    clicklp.clock(click * rand * .3f);
    noise = clicklp.bp();
    noise *= clickvol;
    output = mattack * (2-VCA) * (output+noise);
    VCA += clickattack;
  }

  // generate release click
  if (vca_phase == VP_R && click > 0 && sustain == 0) {
    static unsigned long randSeed = 22222;
    float rand = 0;
    randSeed = (randSeed * 196314165) + 907633515;
    rand = (float)randSeed / std::numeric_limits<unsigned long>::max();
    clicklp.clock(click * rand * .3f);
    noise = clicklp.bp() * clickvol * .7f;
    output += noise;
  }
      
  
  // generate percussion sound
  if (perc_ok && perc_phase > 0) {
    
    s0 = s0-a * s1;		// calculate sine wave
    s1 = s1+a * s0;		//
    output += perc * s0 * perc_vca * perc_vca;
  
    // update percussion envelope every 5 samples
    samplecount2++;
    if (samplecount2 > 5) {
      samplecount2 = 0;
      
      // percussion attack
      if (perc_phase == 1) {
	perc_vca += adsr_attack;
	if (perc_vca >= 1)
	  perc_phase = 2;	// switch to percussion decay
      }
      
      // percussion decay 
      else if (perc_phase == 2) {
	perc_vca -= perc_decay;
	if (perc_vca <= 0) {
	  perc_vca = 0;
	  perc_phase = 0;	// percussion finished
	}
      }
      
      // percussion fast release
      else if (perc_phase == 3)	{
	perc_vca -= adsr_fast_release;
	if (perc_vca <= 0) {
	  perc_vca = 0;
	  perc_phase = 0;	// percussion finished
	}
      }
    }
  }

  output = DENORMALIZE(output);
  return output;
}


long voice::get_note() {
  return(actual_note);
}


bool voice::check_note(long note) {
  if (note == actual_note || note == next_note)
    return(true);
  else
    return(false);
}


bool voice::get_active() {
  if (status == VS_IDLE)
    return(false);
  else
    return(true);
}


void voice::note_on(long note, long velocity, volatile float *table, int size, 
		    float pitch, bool percenable, float sclick, float sust) {
  
  my_table = table;
  my_size = size;
  click = sclick;
  perc_ok = percenable;
  sustain = sust;

  if (note >= 0 && note < 128) {
    if (click > 0) {
      float clickfreq = (freqtab[note] + 70) * 16;
      if (clickfreq > 5000)
	clickfreq = 5000;
      //clicklp.setparam(3000+clickfreq * .3f,.1f,samplerate);
      clicklp.setparam(clickfreq, .1f, samplerate);
      clickvol = note * note * .0008f;
    }
    if (actual_note >= 0) {	// fast retrigger
      next_note = note;
      vca_phase = VP_FR;
      perc_phase = 3;
    }
    else {				// normal note on
      VCA = 0;
      phase = 0;
      phaseinc = 0;
      vca_phase = VP_A;

      actual_note = note & 0x7f;
      perc_phase = 0;
      this->voicecalc();
      perc_phase = 1;
    }

    status = VS_PLAYING;
  }
  else
    note_off(-1);
}


void voice::note_off(long note) {
  if (note == actual_note && next_note >= 0) {
    vca_phase = VP_FR;
    return;
  }

  if (!pedal)
    vca_phase = VP_R;
  else
    status = VS_WAIT_PUP;
	
}


void voice::force_off() {
  next_note = -1;
  vca_phase = VP_FR;
}


void voice::set_pedal(bool pedal) {
  if (pedal != this->pedal) {
    if (!pedal)
      if (status == VS_WAIT_PUP)
	vca_phase = VP_R;

    this->pedal = pedal;
  }
} 


void voice::set_percussion(float percussion, float perc_multiplier,
			   float percfade) {
  if (percussion >= 0)
    perc = percussion * 2;
  if (perc_multiplier >= 0)
    percmultiplier = perc_multiplier;
  if (percfade >= 0)
    perc_fade = 1-percfade+.5f;
}


void voice::set_pitch(float pitch) {
  this->pitch = pitch;
  this->voicecalc();
}


notemaster::notemaster(int number) {
  my_samplerate = 44100;
  pitch = next_pitch = 1;
  my_percussion = -1;
  if (number < 1)
    number = 1;
  if (number>MAXVOICES)
    number = MAXVOICES;

  for (x = 0; x <= MAXVOICES;x++)
    voices[x] = NULL;

  for (x = 0; x <= number;x++) {
    voices[x] = new voice();
    if (voices[x] != NULL) {
      age[x] = 0;
      chan[x] = 15;
      voices[x]->reset();
      voices[x]->set_samplerate(my_samplerate);
      volume[x] = 1;
    }
  }

  numofvoices = number;
}


notemaster::~notemaster() {
  for (x = 0; x < MAXVOICES;x++)
    if (voices[x] != NULL)
      delete voices[x];
}


void notemaster::set_numofvoices(int number) {
  // we create one additional voice for channel 2 (bass pedal)
  if (number < 1)
    number = 1;
  if (number>MAXVOICES)
    number = MAXVOICES;
  for (x = 0; x <= MAXVOICES;x++) {
    if (voices[x] != NULL) {
      delete voices[x];
    }
    voices[x] = NULL;
    age[x] = 0;
  }
  for (x = 0; x <= number;x++) {
    voices[x] = new voice();
    if (voices[x] != NULL) {
      age[x] = 0;
      chan[x] = 15;
      voices[x]->reset();
      voices[x]->set_samplerate(my_samplerate);
      voices[x]->set_percussion(my_percussion,my_perc_multiplier,my_percfade);
      volume[x] = 1;
    }
  }
  numofvoices = number;
}


void notemaster::note_on(long note, long velocity, volatile float *table, 
			 int size1, int channel, bool percenable, 
			 float click, float sustain) {
  /*
    The most interesting part here is the note priority and "stealing"
    algorithm. We do it this way:
    If a new note on event is received we look for an idle voice. If
    there's none available we choose the oldest active voice and let it
    perform a fast note off followed by a note on. This "fast retrigger"
    is entirely calculated by the voice itself.

    "Mono" mode is defined by numofvoices=1.
  */
  int maxpos = 0, newpos;
  unsigned long	maxage;

  note -= 12;

  if (note < 0)
    return;
  if (note > 128)
    return;

  if (channel == 2) {
    newpos = numofvoices;	// reserved voice for bass pedal (channel 2)
  }
  else {
    maxage = 0;
    newpos = -1;
    maxpos = 0;

    // calculate newpos - the voice number to produce the note
    for (x = 0; x < numofvoices; x++) {
      if (voices[x] == NULL)
	continue;

      // do we have an existing note?
      // -> retrigger
      if (voices[x]->get_active() && voices[x]->check_note(note) && 
	  chan[x] == (unsigned char)channel) {
	newpos = x;
	age[x] = 0;
      }

      if (voices[x]->get_active()) {
	// age all active voices
	age[x]++;

	// let maxpos hold number of oldest voice
	if (age[x]>maxage)
	  {
	    maxpos = x;
	    maxage = age[x];
	  }
      }
      else if (newpos ==  -1)	// voice is not active. If we don't have
	// to retrigger - this is our voice!
	newpos = x;
    }

    if (newpos ==  -1)		// no free voice and nothing to retrigger
      newpos = maxpos;	// -> choose oldest voice
  }

  // let the voice play the note. Fast retrigger is handled by the voice.
  voices[newpos]->note_on(note,velocity,table,size1,pitch,percenable,click,sustain);
  age[newpos] = 0;
  if (channel>0 && channel < 3)
    chan[newpos] = (unsigned char)channel;
  else
    chan[newpos] = 0;
}


void notemaster::note_off(long note, int channel) {
  note -= 12;

  if (note < 0)
    return;

  if (channel == 2) {
    if (voices[numofvoices]->check_note(note))
      voices[numofvoices]->note_off(note);
  }
  else {
    for (x = 0; x < numofvoices;x++) {
      if (chan[x] == (unsigned char)channel && voices[x]->check_note(note)) {
	voices[x]->note_off(note);
	age[x] = 0;
      }
    }
  }
}


float *notemaster::clock() {
  output[0] = output[1] = output[2] = 0;
  for (x = 0; x <= numofvoices;x++)
    if (chan[x] < 3)
      output[chan[x]] += volume[chan[x]] * voices[x]->clock();
  //	output[0]=DENORMALIZE(output[0]);
  //	output[1]=DENORMALIZE(output[1]);

  return(output);
}


void notemaster::all_notes_off() {
  pitch = next_pitch = 1;
  for (x = 0; x <= numofvoices;x++) {
    voices[x]->force_off();
    age[x] = 0;
  }
}


void notemaster::set_pedal(int pedal, int channel) {
  bool	my_pedal;
  if (pedal < 64)
    my_pedal = false;
  else
    my_pedal = true;
  for (x = 0; x <= numofvoices;x++)
    voices[x]->set_pedal(my_pedal);
}


void notemaster::set_samplerate(float samplerate) {
  my_samplerate = samplerate;
  for (x = 0; x <= numofvoices;x++)
    voices[x]->set_samplerate(samplerate);
}


void notemaster::set_percussion(float percussion,float perc_multiplier,float percfade) {
  my_percussion = percussion;			// memorize percussion values
  my_perc_multiplier = perc_multiplier;
  my_percfade = percfade;

  for (x = 0; x < numofvoices;x++)
    voices[x]->set_percussion(percussion,perc_multiplier,percfade);

  voices[numofvoices]->set_percussion(percussion * .3f, perc_multiplier,
				      percfade);
}


void notemaster::set_pitch(float pitch, int channel) {
  this->pitch = pitch;
  for (x = 0; x <= numofvoices;x++) {
    if (chan[x] == channel)
      voices[x]->set_pitch(pitch);
  }
}


void notemaster::set_volume(float vol, int channel) {
  volume[channel] = vol;
}


void notemaster::reset() {
  for (x = 0; x <= numofvoices;x++)
    voices[x]->reset();
}


void notemaster::suspend() {
  for (x = 0; x <= numofvoices;x++)
    voices[x]->suspend();
}


void notemaster::resume() {
  for (x = 0; x <= numofvoices;x++)
    voices[x]->resume();
}
