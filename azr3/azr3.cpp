/****************************************************************************
    
    AZR-3 - An LV2 synth plugin
    
    Copyright (C) 2006-2007 Lars Luthman <lars.luthman@gmail.com>
    
    based on source code from the VST plugin AZR-3, (C) 2006 Philipp Mott
    
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

#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>

#include <jack/midiport.h>

#include "azr3.hpp"
#include "voice_classes.h"


using namespace std;


AZR3::AZR3(double rate)
  : n1(NUMOFVOICES),
    samplerate(rate),
    rate_scale(rate / 44100.0),
    vdelay1(int(441 * rate_scale), true),
    vdelay2(int(441 * rate_scale), true),
    wand_r(int(4410 * rate_scale), false),
    wand_l(int(4410 * rate_scale), false),
    delay1(int(4410 * rate_scale), true),
    delay2(int(4410 * rate_scale), true),
    delay3(int(4410 * rate_scale),true),
    delay4(int(4410 * rate_scale),true),
    vlfo((float)rate),
    lfo1((float)rate),
    lfo2((float)rate),
    lfo3((float)rate),
    lfo4((float)rate),
    samplecount(0),
    viblfo(0),
    vmix1(0),
    vmix2(0),
    oldmrvalve(0),
    oldmix(0),
    fuzz(0),
    odmix(0),
    n_odmix(1 - odmix),
    n2_odmix(2 - odmix),
    oldspread(0),
    spread(0),
    spread2(0),
    cross1(0),
    lspeed(0),
    uspeed(0),
    er_r(0),
    er_l(0),
    er_r_before(0),
    er_feedback(0),
    llfo_out(0),
    llfo_nout(0),
    llfo_d_out(0),
    llfo_d_nout(0),
    lfo_out(0),
    lfo_nout(0),
    lfo_d_out(0),
    lfo_d_nout(0),
    splitpoint(0),
    last_shape(-1),
    //mute(true),
    pedal(false),
    odchanged(true),
    last_r(0),
    last_l(0) {
  
  for (int x = 0; x < kNumParams + 3; ++x)
    m_ports[x] = 0;
  
  pthread_mutex_init(&m_notemaster_lock, 0);
  
  for(int x = 0; x < WAVETABLESIZE * 12 + 1; x++)
    wavetable[x] = 0;

  for(int x = 0; x < kNumParams; x++) {
    last_value[x] = -99;
    slow_controls[x] = false;
    m_values[x].old_value = -99;
    m_values[x].new_value = -99;
  }
  slow_controls[n_mono] = true;
  for (int x = 0; x < 9; ++x)
    slow_controls[n_1_db1 + x] = true;
  for (int x = 0; x < 9; ++x)
    slow_controls[n_2_db1 + x] = true;
  for (int x = 0; x < 5; ++x)
    slow_controls[n_3_db1 + x] = true;
  slow_controls[n_shape] = true;
  
  warmth.setparam(2700, 1.2f, samplerate);

  n1.set_samplerate(samplerate);
  vdelay1.set_samplerate(samplerate);
  vdelay2.set_samplerate(samplerate);
  vlfo.set_samplerate(samplerate);
  vlfo.set_rate(35, 0);
  split.setparam(400, 1.3f, samplerate);
  horn_filt.setparam(2500, .5f, samplerate);
  damp.setparam(200, .9f, samplerate);
  wand_r.set_samplerate(samplerate);
  wand_r.set_delay(35);
  wand_l.set_samplerate(samplerate);
  wand_l.set_delay(20);

  delay1.set_samplerate(samplerate);
  delay2.set_samplerate(samplerate);
  delay3.set_samplerate(samplerate);
  delay4.set_samplerate(samplerate);
  lfo1.set_samplerate(samplerate);
  lfo2.set_samplerate(samplerate);
  lfo3.set_samplerate(samplerate);
  lfo4.set_samplerate(samplerate);

  body_filt.setparam(190, 1.5f, samplerate);
  postbody_filt.setparam(1100, 1.5f, samplerate);

}


AZR3::~AZR3() {
  pthread_mutex_destroy(&m_notemaster_lock);
}


void AZR3::activate() {

  //mute = false;

  for(int x = 0; x < 4; x++) {
    allpass_r[x].reset();
    allpass_l[x].reset();
  }
  
  delay1.flood(0);
  delay2.flood(0);
  delay3.flood(0);
  delay4.flood(0);
  vdelay1.flood(0);
  vdelay2.flood(0);
  wand_r.flood(0);
  wand_l.flood(0);
  
  sem_init(&m_qsem, 0, 1);
  pthread_create(&m_worker, 0, &AZR3::worker_function, this);
}


void AZR3::deactivate() {
  pthread_cancel(m_worker);
  pthread_join(m_worker, 0);
  sem_destroy(&m_qsem);
}


void AZR3::connect_port(uint32_t port, void* buffer) {
  m_ports[port] = buffer;
}


void AZR3::run(uint32_t sampleFrames) {
  
  /*
    OK, here we go. This is the order of actions in here:
    - process event queue
    - return zeroes if in "mute" state
    - clock the "notemaster" and get the combined sound output
    from the voices.
    We actually get three values, one for each keyboard.
    They are added according to the assigned channel volume
    control values.
    - calculate switch smoothing to prevent clicks
    - vibrato
    - additional low pass "warmth"
    - distortion
    - speakers
  */
  
  float* out1 = p(64);
  float* out2 = p(65);
  
  // if the notemaster mutex is locked, don't try to render anything
  if (pthread_mutex_trylock(&m_notemaster_lock)) {
    memset(out1, 0, sizeof(float) * sampleFrames);
    memset(out2, 0, sizeof(float) * sampleFrames);
    return;
  }
  
  // send slow port changes to the worker thread
  if (!sem_trywait(&m_qsem)) {
    for (int i = 0; i < kNumParams; ++i) {
      if (slow_controls[i])
	m_values[i].new_value = *p(i);
    }
    sem_post(&m_qsem);
  }
  
  // vibrato handling
  if (*p(n_1_vibrato) != last_value[n_1_vibrato]) {
    vibchanged1 = true;
    last_value[n_1_vibrato] = *p(n_1_vibrato);
  }
  if (*p(n_2_vibrato) != last_value[n_2_vibrato]) {
    vibchanged2 = true;
    last_value[n_2_vibrato] = *p(n_2_vibrato);
  }
  if (*p(n_1_vmix) != last_value[n_1_vmix]) {
    if (*p(n_1_vibrato) == 1) {
      vmix1 = *p(n_1_vmix);
      vibchanged1 = true;
    }
    last_value[n_1_vmix] = *p(n_1_vmix);
  }
  if (*p(n_2_vmix) != last_value[n_2_vmix]) {
    if (*p(n_2_vibrato) == 1) {
      vmix2 = *p(n_2_vmix);
      vibchanged2 = true;
    }
    last_value[n_2_vmix] = *p(n_2_vmix);
  }
  
  // compute click
  calc_click();
  
  // set percussion parameters
  {
    int v = (int)(*p(n_perc) * 10);
    float pmult;
    if(v < 1)
      pmult = 0;
    else if(v < 2)
      pmult = 1;
    else if(v < 3)
      pmult = 2;
    else if(v < 4)
      pmult = 3;
    else if(v < 5)
      pmult = 4;
    else if(v < 6)
      pmult = 6;
    else if(v < 7)
      pmult = 8;
    else if(v < 8)
      pmult = 10;
    else if(v < 9)
      pmult = 12;
    else
      pmult = 16;
    n1.set_percussion(1.5f * *p(n_percvol), pmult, *p(n_percfade));
  }
  
  // set volumes
  n1.set_volume(*p(n_vol1) * 0.3f, 0);
  n1.set_volume(*p(n_vol2) * 0.4f, 1);
  n1.set_volume(*p(n_vol3) * 0.6f, 2);
  
  // has the distortion switch changed?
  if (*p(n_mrvalve) != oldmrvalve) {
    odchanged = true;
    oldmrvalve = *p(n_mrvalve);
  }
  
  // compute distortion parameters
  float value = *p(n_drive);
  bool do_dist;
  if (value > 0)
    do_dist = true;
  else
    do_dist = false;
  float dist = 2 * (0.1f + value);
  float sin_dist = sinf(dist);
  float i_dist = 1 / dist;
  float dist4 = 4 * dist;
  float dist8 = 8 * dist;
  fuzz_filt.setparam(800 + *p(n_tone) *
                     3000, 0.7f, samplerate);
  value = *p(n_mix);
  if (value != oldmix) {
    odmix = value;
    if (*p(n_mrvalve) == 1)
      odchanged = true;
  }

  // speed control port
  bool fastmode;
  if (*p(n_speed) > 0.5f)
    fastmode = true;
  else
    fastmode = false;
  
  // different rotation speeds
  float lslow = 10 * *p(n_l_slow);
  float lfast = 10 * *p(n_l_fast);
  float uslow = 10 * *p(n_u_slow);
  float ufast = 10 * *p(n_u_fast);
  
  // belt (?)
  value = *p(n_belt);
  float ubelt_up = (value * 3 + 1) * 0.012f;
  float ubelt_down = (value * 3 + 1) * 0.008f;
  float lbelt_up = (value * 3 + 1) * 0.0045f;
  float lbelt_down = (value * 3 + 1) * 0.0035f;
  
  if (oldspread != *p(n_spread)) {
    lfos_ok = false;
    oldspread = *p(n_spread);
  }
  
  // keyboard split
  splitpoint = (long)(*p(n_splitpoint) * 128);
  
  jack_nframes_t event_index = 0;
  void* midi = p<void>(63);
  jack_nframes_t event_count = jack_midi_get_event_count(midi);
  int     x;
  float last_out1, last_out2;
  unsigned char* evt;
  jack_midi_event_t event;
  uint32_t pframe = 0;

  while (event_index <= event_count) {
    
    if (event_index == event_count)
      event.time = sampleFrames;
    else
      jack_midi_event_get(&event, midi, event_index);
    ++event_index;
      
    for ( ; pframe < event.time; ++pframe) {
      
      // we need this variable further down
      ++samplecount;
      if(samplecount > 10000)
	samplecount = 0;
      
      // if n_pedalspeed is on, use the hold pedal for speed
      if (*p(n_pedalspeed) >= 0.5)
	fastmode = pedal;
      
      float* p_mono = n1.clock();
      float mono1 = p_mono[0];
      float mono2 = p_mono[1];
      float mono = p_mono[2];
      
      // smoothing of vibrato switch 1
      if (vibchanged1 && samplecount % 10 == 0) {
	if(*p(n_1_vibrato) == 1) {
	  vmix1 += 0.01f;
	  if (vmix1 >= *p(n_1_vmix))
	    vibchanged1 = false;
	}
	else {
	  vmix1 -= 0.01f;
	  if (vmix1 <= 0)
	    vibchanged1 = false;
	}
      }
      
      // smoothing of vibrato switch 2
      if(vibchanged2 && samplecount % 10 == 0) {
	if(*p(n_2_vibrato) == 1) {
	  vmix2 += 0.01f;
	  if (vmix2 >= *p(n_2_vmix))
	    vibchanged2 = false;
	}
	else {
	  vmix2 -= 0.01f;
	  if (vmix2 <= 0)
	    vibchanged2 = false;
	}
      }
      
      // smoothing of OD switch
      if(odchanged && samplecount % 10 == 0) {
	if(*p(n_mrvalve) > 0.5) {
	  odmix += 0.05f;
	  if (odmix >= *p(n_mix))
	    odchanged = false;
	}
	else {
	  odmix -= 0.05f;
	  if (odmix <= 0)
	    odchanged = false;
	}
	n_odmix = 1 - odmix;
	n2_odmix = 2 - odmix;
	odmix75 = 0.75f * odmix;
	n25_odmix = n_odmix * 0.25f;
      }
      
      // Vibrato LFO
      bool lfo_calced = false;
      
      // Vibrato 1
      if(*p(n_1_vibrato) == 1 || vibchanged1) {
	if(samplecount % 5 == 0) {
	  viblfo = vlfo.clock();
	  lfo_calced = true;
	  vdelay1.set_delay(viblfo * 2 * *p(n_1_vstrength));
	}
	mono1 = (1 - vmix1) * mono1 + vmix1 * vdelay1.clock(mono1);
      }
      
      // Vibrato 2
      if(*p(n_2_vibrato) == 1 || vibchanged2) {
	if(samplecount % 5 == 0) {
	  if(!lfo_calced)
	    viblfo = vlfo.clock();
	  vdelay2.set_delay(viblfo * 2 * *p(n_2_vstrength));
	}
	mono2 = (1 - vmix2) * mono2 + vmix2 * vdelay2.clock(mono2);
      }
      
      mono += mono1 + mono2;
      mono *= 1.4f;
      
      // Mr. Valve
      /*
	Completely rebuilt.
	Multiband distortion:
	The first atan() waveshaper is applied to a lower band. The second
	one is applied to the whole spectrum as a clipping function (combined
	with an fabs() branch).
	The "warmth" filter is now applied _after_ distortion to flatten
	down distortion overtones. It's only applied with activated distortion
	effect, so we can switch warmth off and on without adding another 
	parameter.
      */
      if (*p(n_mrvalve) > 0.5 || odchanged) {
	if (do_dist) {
	  body_filt.clock(mono);
	  postbody_filt.clock(atanf(body_filt.lp() * dist8) * 6);
	  fuzz = atanf(mono * dist4) * 0.25f + 
	    postbody_filt.bp() + postbody_filt.hp();
	  
	  if (_fabsf(mono) > *p(n_set))
	    fuzz = atanf(fuzz * 10);
	  fuzz_filt.clock(fuzz);
	  mono = ((fuzz_filt.lp() * odmix * sin_dist + mono * (n2_odmix)) * 
		  sin_dist) * i_dist;
	}
	else {
	  fuzz_filt.clock(mono);
	  mono = fuzz_filt.lp() * odmix75 + mono * n25_odmix * i_dist;
	}
	mono = warmth.clock(mono);
      }
      
      // Speakers
      /*
	I started the rotating speaker sim from scratch with just
	a few sketches about how reality looks like:
	Two horn speakers, rotating in a circle. Combined panning
	between low and mid filtered sound and the volume. Add the
	doppler effect. Let the sound of one speaker get reflected
	by a wall and mixed with the other speakers' output. That's
	all not too hard to calculate and to implement in C++, and
	the results were already quite realistic. However, to get
	more density and the explicit "muddy" touch I added some
	phase shifting gags and some unexpected additions with
	the other channels' data. The result did take many nights
	of twiggling wih parameters. There are still some commented
	alternatives; feel free to experiment with the emulation.
	Never forget to mono check since there are so many phase
	effects in here you might end up in the void.
	I'm looking forward to the results...
      */
      
      /*
	Update:
	I added some phase shifting using allpass filters.
	This should make it sound more realistic.
      */
      
      if (*p(n_speakers) > 0.5) {
	if (samplecount % 100 == 0) {
	  if (fastmode) {
	    if (lspeed < lfast)
	      lspeed += lbelt_up;
	    if (lspeed > lfast)
	      lspeed = lfast;
	    
	    if (uspeed < ufast)
	      uspeed += ubelt_up;
	    if (uspeed > ufast)
	      uspeed = ufast;
	  }
	  else {
	    if (lspeed > lslow)
	      lspeed -= lbelt_down;
	    if (lspeed < lslow)
	      lspeed = lslow;
	    if (uspeed > uslow)
	      uspeed -= ubelt_down;
	    if (uspeed < uslow)
	      uspeed = uslow;
	  }
	  
	  //recalculate mic positions when "spread" has changed
	  if(!lfos_ok) {
	    float s = (*p(n_spread) + 0.5f) * 0.8f;
	    spread = (s) * 2 + 1;
	    spread2 = (1 - spread) / 2;
	    // this crackles - use offset_phase instead
	    //lfo1.set_phase(0);
	    //lfo2.set_phase(s / 2);
	    //lfo3.set_phase(0);
	    //lfo4.set_phase(s / 2);
	    lfo2.offset_phase(lfo1, s / 2);
	    lfo4.offset_phase(lfo3, s / 2);
	    
	    cross1 = 1.5f - 1.2f * s;
	    // early reflections depend upon mic position.
	    // we want less e/r if mics are positioned on
	    // opposite side of speakers.
	    // when positioned right in front of them e/r
	    // brings back some livelyness.
	    //
	    // so "spread" does the following to the mic positions:
	    // minimum: mics are almost at same position (mono) but
	    // further away from cabinet.
	    // maximum: mics are on opposite sides of cabinet and very
	    // close to speakers.
	    // medium: mics form a 90° angle, heading towards cabinet at
	    // medium distance.
	    er_feedback = 0.03f * cross1;
	    lfos_ok = true;
	  }
	  
	  if (lspeed != lfo3.get_rate()) {
	    lfo3.set_rate(lspeed * 5, 1);
	    lfo4.set_rate(lspeed * 5, 1);
	  }
	  
	  if (uspeed != lfo1.get_rate()) {
	    lfo1.set_rate(uspeed * 5, 1);
	    lfo2.set_rate(uspeed * 5, 1);
	  } 
	}
	
	// split signal into upper and lower cabinet speakers
	split.clock(mono);
	float lower = split.lp() * 5;
	float upper = split.hp();
	
	// upper speaker is kind of a nasty horn - this makes up
	// a major part of the typical sound!
	horn_filt.clock(upper);
	upper = upper * 0.5f + horn_filt.lp() * 2.3f;
	damp.clock(upper);
	float upper_damp = damp.lp();
	
	// do lfo stuff
	if(samplecount % 5 == 0) {
	  lfo_d_out = lfo1.clock();
	  lfo_d_nout = 1 - lfo_d_out;
	  
	  delay1.set_delay(10 + lfo_d_out * 0.8f);
	  delay2.set_delay(17 + lfo_d_nout * 0.8f);
	  
	  lfo_d_nout = lfo2.clock();
	  
	  lfo_out = lfo_d_out * spread + spread2;
	  lfo_nout = lfo_d_nout * spread + spread2;
	  
	  // phase shifting lines
	  // (do you remember? A light bulb and some LDRs...
	  //  DSPing is so much nicer than soldering...)
	  float lfo_phaser1 = (1 - cosf(lfo_d_out * 1.8f) + 1) * 0.054f;
	  float lfo_phaser2 = (1 - cosf(lfo_d_nout * 1.8f) + 1) * .054f;
	  for(x = 0; x < 4; x++) {
	    allpass_r[x].set_delay(lfo_phaser1);
	    allpass_l[x].set_delay(lfo_phaser2);
	  }
	  
	  if(lslow > 0) {
	    llfo_d_out = lfo3.clock();
	    llfo_d_nout = 1 - llfo_d_out;
	  }
	  
	  // additional delay lines in complex mode
	  if(*p(n_complex) > 0.5f) {
	    delay4.set_delay(llfo_d_out + 15);
	    delay3.set_delay(llfo_d_nout + 25);
	  }
	  
	  llfo_d_nout = lfo4.clock();
	  llfo_out = llfo_d_out * spread + spread2;
	  llfo_nout = llfo_d_nout * spread + spread2;
	}
	
	float lright, lleft;
	if(lslow > 0) {
	  lright = (1 + 0.6f * llfo_out) * lower;
	  lleft = (1 + 0.6f * llfo_nout) * lower;
	}
	else {
	  lright = lleft = lower;
	}
	
	// emulate vertical horn characteristics
	// (sound is dampened when listened from aside)
	float right = (3 + lfo_nout * 2.5f) * upper + 1.5f * upper_damp;
	float left = (3 + lfo_out * 2.5f) * upper + 1.5f * upper_damp;
	
	//phaser...
	last_r = allpass_r[0].clock(
	         allpass_r[1].clock(
	         allpass_r[2].clock(
	         allpass_r[3].clock(upper + last_r * 0.33f))));
	last_l = allpass_l[0].clock(
	         allpass_l[1].clock(
	         allpass_l[2].clock(
	         allpass_l[3].clock(upper + last_l * 0.33f))));

	right += last_r;
	left += last_l;
	
	// rotating speakers can only develop in a live room -
	// wouldn't work without some early reflections.
	er_r = wand_r.clock(right + lright - (left *0.3f) - er_l * er_feedback);
	er_r = DENORMALIZE(er_r);
	er_l = wand_l.clock(left + lleft - (right * .3f) - 
			    er_r_before * er_feedback);
	er_l = DENORMALIZE(er_l);
	er_r_before = er_r;
	
	
	// We use two additional delay lines in "complex" mode
	if (*p(n_complex) > 0.5f) {
	  right = right * 0.3f + 1.5f * er_r + 
	    delay1.clock(right) + delay3.clock(er_r);
	  left = left * 0.3f + 1.5f * er_l + 
	    delay2.clock(left) + delay4.clock(er_l);
	}
	else {
	  right = right * 0.3f + 1.5f * er_r + delay1.clock(right) + lright;
	  left = left * 0.3f + 1.5f * er_l + delay2.clock(left) + lleft;
	}
	
	right *= 0.033f;
	left *= 0.033f;
	
	// spread crossover (emulates mic positions)
	last_out1 = (left + cross1 * right) * *p(n_master);
	last_out2 = (right + cross1 * left) * *p(n_master);
      }
      else {
	last_out1 = last_out2 = mono * *p(n_master);
      }
      //if(mute) {
      //  last_out1 = 0;
      //  last_out2 = 0;
      //}
      
      (*out1++) = last_out1;
      (*out2++) = last_out2;
    }
    
    // Handle MIDI event
    if (event.time < sampleFrames) {
      evt = event.buffer;
      unsigned char status = evt[0] & 0xF0;
      if (event.size >= 3 && status >= 0x80 && status <= 0xE0) {
	unsigned char channel = evt[0] & 0x0F;
	volatile float* tbl;
	
	// do the keyboard split
	if ((status == 0x80 || status == 0x90) &&
	    splitpoint > 0 && channel == 0 && evt[1] <= splitpoint)
	  channel = 2;
	
	switch (status) {
	case evt_noteon: {
	  unsigned char note = evt[1];
	  bool percenable = false;
	  float sustain = *p(n_sustain) + .0001f;
	  
	  // here we choose the correct wavetable according to the played note
#define foldstart 80
	  if (note > foldstart + 12 + 12)
	    tbl = &wavetable[channel * WAVETABLESIZE * TABLES_PER_CHANNEL + 
			     WAVETABLESIZE * 7];
	  else if (note > foldstart + 12 + 8)
	    tbl = &wavetable[channel * WAVETABLESIZE * TABLES_PER_CHANNEL +
			     WAVETABLESIZE * 6];
	  else if (note > foldstart + 12 + 5)
	    tbl = &wavetable[channel * WAVETABLESIZE * TABLES_PER_CHANNEL +
			     WAVETABLESIZE * 5];
	  else if (note > foldstart + 12)
	    tbl = &wavetable[channel * WAVETABLESIZE * TABLES_PER_CHANNEL +
			     WAVETABLESIZE * 4];
	  else if (note > foldstart + 8)
	    tbl = &wavetable[channel * WAVETABLESIZE * TABLES_PER_CHANNEL + 
			     WAVETABLESIZE * 3];
	  else if (note > foldstart + 5)
	    tbl = &wavetable[channel * WAVETABLESIZE * TABLES_PER_CHANNEL +
			     WAVETABLESIZE * 2];
	  else if (note > foldstart)
	    tbl = &wavetable[channel * WAVETABLESIZE * TABLES_PER_CHANNEL +
			     WAVETABLESIZE];
	  else
	    tbl = &wavetable[channel * WAVETABLESIZE * TABLES_PER_CHANNEL];
	  
	  if (channel == 0) {
	    if (*p(n_1_perc) > 0)
	      percenable = true;
	    if (*p(n_1_sustain) < 0.5f)
	      sustain = 0;
	  }
	  else if (channel == 1) {
	    if (*p(n_2_perc) > 0)
	      percenable = true;
	    if (*p(n_2_sustain) < 0.5f)
	      sustain = 0;
	  }
	  else if (channel == 2) {
	    if (*p(n_3_perc) > 0)
	      percenable = true;
	    if (*p(n_3_sustain) < 0.5f)
	      sustain = 0;
	  }
	  
	  n1.note_on(note, evt[2], tbl, WAVETABLESIZE, 
		     channel, percenable, click[channel], sustain);
	  
	  break;
	}
	  
	case evt_noteoff:
	  n1.note_off(evt[1], channel);
	  break;
	  
	case 0xB0:
	  
	  // all notes off
	  if (evt[1] >= 0x78 && evt[1] <= 0x7F)
	    n1.all_notes_off();
	  
	  // hold pedal
	  else if (evt[1] == 0x40) {
	    pedal = evt[2] >= 64;
	    if (*p(n_pedalspeed) < 0.5)
	      n1.set_pedal(evt[2], channel);
	  }
	  break;
	  
	case evt_pitch: {
	  float bender = *p(n_bender);
	  float pitch = (float)(evt[2] * 128 + evt[1]);
	  if (pitch > 8192 + 600) {
	    float p = pitch / 8192 - 1;
	    pitch = p * (float)pow(1.059463094359, int(12 * bender)) + 1 - p;
	  }
	  else if(pitch < 8192 - 600) {
	    float p = (8192 - pitch) / 8192;
	    pitch = 1 / (p * (float)pow(1.059463094359, 
					int(12 * bender)) + 1 - p);
	  }
	  else
	    pitch = 1;
	  n1.set_pitch(pitch, channel);
	  break;
	}
	  
	}
      }
    }


  }
  
  pthread_mutex_unlock(&m_notemaster_lock);
}


bool AZR3::make_waveforms(int shape) {
  
  long    i;
  float   amp = 0.5f;
  float   tw = 0, twfuzz;
  float   twmix = 0.5f;
  float   twdist = 0.7f;
  float   tws = (float)WAVETABLESIZE;
        
        
  if(shape == last_shape)
    return false;
  last_shape = shape;
  /*
    We don't just produce flat sine curves but slightly distorted
    and even a triangle wave can be choosen.
    
    Though this makes things sound more interesting it's _not_ a
    tonewheel emulation. If we wanted that we would have to calculate
    or otherwise define different waves for every playable note. If
    anyone wants to implement real tonewheels you will have to make
    drastic changes:
    - implement many wavetables and a choosing algorithm
    - construct wavetable data either through calculations
    or from real wave files. Tha latter is what they do at
    n@tive 1nstrument5.
  */
  for (i = 0; i < WAVETABLESIZE; i++) {
    float   ii = (float)i;
                
    if (shape == W_SINE1 || shape == W_SINE2 || shape == W_SINE3) {
      tw = amp *
	(sinf(ii * 2 * Pi / tws) +
         0.03f * sinf(ii * 8 * Pi / tws) +
         0.01f * sinf(ii * 12 * Pi / tws));
                        
      if (shape == W_SINE2)
	twdist = 1;
      else if (shape == W_SINE3)
	twdist = 2;
                        
      tw *= twdist;
      twfuzz = 2 * tw - tw * tw * tw;
      if (twfuzz > 1)
	twfuzz = 1;
      else if (twfuzz < -1)
	twfuzz = -1;
      tonewheel[i] = 0.5f * twfuzz / twdist;
    }
    else if (shape == W_TRI) {
      if (i < int(tws / 4) || i > int(tws * 0.75f))
	tw += 2 / tws;
      else
	tw -= 2 / tws;
      tonewheel[i] = tw;
    }
    else if (shape == W_SAW) {
      tw = sinf(ii * Pi / tws);
      if (i > int(tws / 2))   {
	tw = sinf((ii - tws / 2) * Pi / tws);
	tw = 1 - tw;
      }
                        
      tonewheel[i] = tw - 0.5f;
    }
    else {
      tw = amp *
	(sinf(ii * 2 * Pi / tws) +
         0.03f * sinf(ii * 8 * Pi / tws) +
         0.01f * sinf(ii * 12 * Pi / tws));
      tonewheel[i]=tw;
    }
  }
        
  for (i = 0; i < WAVETABLESIZE; i++) {
    //              int     f=TONEWHEELSIZE/WAVETABLESIZE;
    int f = 1;
    int     icount;
    int i2[9];
                
    i2[0] = (int)(i * 1 * f);
    i2[1] = (int)(i * 2 * f);
    i2[2] = (int)(i * 3 * f);
    i2[3] = (int)(i * 4 * f);
    i2[4] = (int)(i * 6 * f);
    i2[5] = (int)(i * 8 * f);
    i2[6] = (int)(i * 10 * f);
    i2[7] = (int)(i * 12 * f);
    i2[8] = (int)(i * 16 * f);
                
    for (icount = 0; icount < 9; icount++) {
      while(i2[icount] >= WAVETABLESIZE)
	i2[icount] -= WAVETABLESIZE;
    }
                
    sin_16[i] = tonewheel[i2[0]];
    sin_8[i] = tonewheel[i2[1]];
    sin_513[i] = tonewheel[i2[2]];
    sin_4[i] = tonewheel[i2[3]];
    sin_223[i] = tonewheel[i2[4]];
    sin_2[i] = tonewheel[i2[5]];
    sin_135[i] = tonewheel[i2[6]];
    sin_113[i] = tonewheel[i2[7]];
    sin_1[i] = tonewheel[i2[8]];
  }
        
  return true;
}


// make one of the three waveform sets with four complete waves
// per set. "number" is 1..3 and references the waveform set
void AZR3::calc_waveforms(int number) {
  
  int i, c;
  volatile float* t;
  float   this_p[kNumParams];

  for (c = 0; c < kNumParams; c++)
    this_p[c] = m_values[c].old_value;
  if (number == 2) {
    c = n_2_db1;
    t = &wavetable[WAVETABLESIZE * TABLES_PER_CHANNEL];
  }
  else if (number == 3) {
    t = &wavetable[WAVETABLESIZE * TABLES_PER_CHANNEL * 2];
    c = n_3_db1;
  }
  else {
    t = &wavetable[0];
    c = n_1_db1;
  }

  // weight to each drawbar
  this_p[c] *= 1.5f;
  this_p[c+1] *= 1.0f;
  this_p[c+2] *= 0.8f;
  this_p[c+3] *= 0.8f;
  this_p[c+4] *= 0.8f;
  this_p[c+5] *= 0.8f;
  this_p[c+6] *= 0.8f;
  this_p[c+7] *= 0.6f;
  this_p[c+8] *= 0.6f;

  for (i = 0; i < WAVETABLESIZE; i++) {
    t[i] = t[i + WAVETABLESIZE] = t[i + WAVETABLESIZE*2] = 
      t[i+WAVETABLESIZE * 3] = t[i+WAVETABLESIZE * 4] =
      t[i+WAVETABLESIZE * 5] = t[i+WAVETABLESIZE * 6] =
      t[i+WAVETABLESIZE * 7] =
      sin_16[i] * this_p[c] + 
      sin_8[i] * this_p[c+1] +
      sin_513[i] * this_p[c+2];

    /*
      This is very important for a warm sound:
      The "tone wheels" are a limited resource and they
      supply limited pitch heights. If a drawbar register
      is forced to play a tune above the highest possible
      note it will simply be transposed one octave down.
      In addition it will appear less loud; that's what
      d2, d4 and d8 are for.
    */
#define d2 0.5f
#define d4 0.25f
#define d8 0.125f
    if(number == 3) {
      t[i] 
        += sin_4[i] * this_p[c+3] + 
        sin_223[i] * this_p[c+4];
      t[i + WAVETABLESIZE * 1] +=
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4];
      t[i + WAVETABLESIZE * 2] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4];
      t[i + WAVETABLESIZE * 3] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4];
      t[i + WAVETABLESIZE * 4] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4];
      t[i + WAVETABLESIZE * 5] += 
        sin_4[i] * this_p[c + 3] +
        sin_223[int(i / 2)] * d2 * this_p[c + 4];
      t[i + WAVETABLESIZE * 6] += 
        sin_4[i] * this_p[c+3] + 
        sin_223[int(i / 2)] * d2 * this_p[c + 4];
      t[i + WAVETABLESIZE * 7] += 
        sin_4[int(i / 2)] * d2 * this_p[c + 3] + 
        sin_223[int(i / 2)] * d2 * this_p[c + 4];
    }
    else {
      t[i] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4] +
	sin_2[i] * this_p[c + 5] +
	sin_135[i] * this_p[c + 6] + 
        sin_113[i] * this_p[c + 7] +
	sin_1[i] * this_p[c + 8];
      t[i + WAVETABLESIZE] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4] +
	sin_2[i] * this_p[c + 5] +
        sin_135[i] * this_p[c + 6] + 
        sin_113[i] * this_p[c + 7] +
        sin_1[int(i / 2)] * d2 * this_p[c + 8];
      t[i + WAVETABLESIZE * 2] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4] +
        sin_2[i] * this_p[c + 5] +
        sin_135[i] * this_p[c + 6] + 
        sin_113[int(i / 2)] * d2 * this_p[c + 7] +
        sin_1[int(i / 2)] * d2 * this_p[c + 8];
      t[i + WAVETABLESIZE * 3] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4] +
        sin_2[i] * this_p[c + 5] +
        sin_135[int(i / 2)] * d2 * this_p[c + 6] + 
        sin_113[int(i / 2)] * d2 * this_p[c + 7] +
        sin_1[int(i / 2)] * d2 * this_p[c + 8];
      t[i + WAVETABLESIZE * 4] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4] +
        sin_2[int(i / 2)] * d2 * this_p[c + 5] +
        sin_135[int(i / 2)] * d2 * this_p[c + 6] + 
        sin_113[int(i / 2)] * d2 * this_p[c + 7] +
        sin_1[int(i / 4)] * d4 * this_p[c + 8];
      t[i + WAVETABLESIZE * 5] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[int(i / 2)] * d2 * this_p[c + 4] +
        sin_2[int(i / 2)] * d2 * this_p[c + 5] +
        sin_135[int(i / 2)] * d2 * this_p[c + 6] + 
        sin_113[int(i / 4)] * d4 * this_p[c + 7] +
        sin_1[int(i / 4)] * d4 * this_p[c + 8];
      t[i + WAVETABLESIZE * 6]  += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[int(i / 2)] * d2 * this_p[c + 4] +
        sin_2[int(i / 2)] * d2 * this_p[c + 5] +
        sin_135[int(i / 4)] * 0 * this_p[c + 6] + 
        sin_113[int(i / 4)] * d4 * this_p[c + 7] +
        sin_1[int(i / 4)] * d4 * this_p[c + 8];
      t[i + WAVETABLESIZE * 7] += 
        sin_4[int(i / 2)] * d2 * this_p[c + 3] + 
        sin_223[int(i / 2)] * d2 * this_p[c + 4] +
        sin_2[int(i / 4)] * d4 * this_p[c + 5] +
        sin_135[int(i / 4)] * 0 * this_p[c + 6] + 
        sin_113[int(i / 4)] * d4 * this_p[c + 7] +
        sin_1[int(i / 8)] * d8 * this_p[c + 8];
    } 
  }
  /*
    The grown up source code viewer will find that sin_135 is only
    folded once (/2). Well, I had terrible aliasing problems when
    folding it twice (/4), and the easiest solution was to set it to
    zero instead. You can't claim you actually heard it, can you?
  */
  wavetable[WAVETABLESIZE * 12] = 0;
}


void AZR3::calc_click() {
  /*
    Click is not just click - it has to follow the underlying
    note pitch. However, the click emulation is just "try and
    error". Improve it if you can, but PLEAZE tell me how you
    did it...
  */
  click[0] = *p(n_click) *
    (*p(n_1_db1) + *p(n_1_db2) + *p(n_1_db3) + *p(n_1_db4) +
     *p(n_1_db5) + *p(n_1_db6) + *p(n_1_db7) + *p(n_1_db8) +
     *p(n_1_db9)) / 9;

  click[1] = *p(n_click) *
    (*p(n_2_db1) + *p(n_2_db2) + *p(n_2_db3) + *p(n_2_db4) +
     *p(n_2_db5) + *p(n_2_db6) + *p(n_2_db7)+*p(n_2_db8) +
     *p(n_2_db9)) / 9;

  click[2] = *p(n_click) *
    (*p(n_3_db1) + *p(n_3_db2) + *p(n_3_db3) + *p(n_3_db4) + 
     *p(n_1_db5)) / 22;
}


void* AZR3::worker_function(void* arg) {
  
  AZR3& me = *static_cast<AZR3*>(arg);
  return me.worker_function_real();
}


void* AZR3::worker_function_real() {

  bool change_mono = false;
  bool change_shape = false;
  bool change_organ1 = false;
  bool change_organ2 = false;
  bool change_organ3 = false;
  
  do {
    
    // sleep for a while - we don't need to update the tables for _every_ 
    // change
    usleep(10000);
    
    // wait until the audio thread is done writing
    sem_wait(&m_qsem);
    
    // read port changes from the queue until the semaphore would block
    for (unsigned i = 0; i < kNumParams; ++i) {
      if (slow_controls[i]) {
	if (m_values[i].old_value != m_values[i].new_value) {
	  if (i == n_mono)
	    change_mono = true;
	  else if (i >= n_1_db1 && i <= n_1_db9)
	    change_organ1 = true;
	  else if (i >= n_2_db1 && i <= n_2_db9)
	    change_organ2 = true;
	  else if (i >= n_3_db1 && i <= n_3_db5)
	    change_organ3 = true;
	  else if (i == n_shape)
	    change_shape = true;
	  
	  m_values[i].old_value = m_values[i].new_value;
        }
      }
    }
    
    // done reading, release the semaphore
    sem_post(&m_qsem);
    
    // act on the port changes
    if (change_mono) {
      pthread_mutex_lock(&m_notemaster_lock);
      if (m_values[n_mono].old_value >= 0.5f)
        n1.set_numofvoices(1);
      else
        n1.set_numofvoices(NUMOFVOICES);
      pthread_mutex_unlock(&m_notemaster_lock);
      change_mono = false;
    }
    
    // this is not strictly safe - we are writing to the wavetable buffer
    // while the audio thread may be reading from it. if you get crackly
    // noises while moving the shape knob or the drawbars this might be 
    // the cause.
    if (change_shape) {
      if (make_waveforms(int(m_values[n_shape].old_value * 
			     (W_NUMOF - 1) + 1) - 1)) {
        calc_waveforms(1);
        calc_waveforms(2);
        calc_waveforms(3);
      }
      change_shape = false;
      change_organ1 = false;
      change_organ2 = false;
      change_organ3 = false;
    }
    
    if (change_organ1) {
      calc_waveforms(1);
      change_organ1 = false;
    }
    
    if (change_organ2) {
      calc_waveforms(2);
      change_organ2 = false;
    }
        
    if (change_organ3) {
      calc_waveforms(3);
      change_organ3 = false;
    } 

  } while (true);
  
  return 0;
}



