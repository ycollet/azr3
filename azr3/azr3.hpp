/****************************************************************************
  
  AZR-3 - An LV2 synth plugin
  
  Copyright (C) 2006-2007 Lars Luthman <lars.luthman@gmail.com>
  
  based on source code from the VST plugin AZR-3, (C) 2006 Philipp Mott
  
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

#ifndef AZR3_HPP
#define AZR3_HPP

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#include "voice_classes.h"
#include "Globals.h"


enum {
  evt_noteon = 0x90,
  evt_noteoff = 0x80,
  evt_alloff,
  evt_pedal,
  evt_pitch = 0xE0,
  evt_modulation,
  evt_volume,
  evt_channel_volume,
  evt_drawbar
};


class AZR3 {
public:
 
  AZR3(double rate);
 
  ~AZR3();
 
  void activate();
  
  void deactivate();
  
  void connect_port(uint32_t port, void* buffer);
  
  void run(uint32_t nframes);
 
protected: 
 
  /** Generate the basic tonewheel waveforms that are used to build the
      three different organ sounds. Should only be called from the worker 
      thread. */
  bool make_waveforms(int shape);
 
  /** Compute one of the three organ sounds using the basic tonewheel waveforms
      and the drawbar settings for that organ section. Should only be called
      from the worker thread. */
  void calc_waveforms(int number);
 
  /** Compute click coefficients. */
  void calc_click();
 
  /** This is a wrapper for worker_function_real(), needed because the
      pthreads API does not know about classes and member functions. The
      parameter should be a pointer to an AZR3 object. */
  static void* worker_function(void* arg);
 
  /** This function is executed in a worker thread and does non-RT safe things
      like computing wavetables. */
  void* worker_function_real();
 
protected:
  
  /** Use this as a shorthand to access and cast port buffers. */
  template <typename T> inline T* p(uint32_t port) {
    return reinterpret_cast<T*>(m_ports[port]);
  }
  
  /** This is needed because default template parameters aren't allowed for
      template functions. */
  inline float* p(uint32_t port) {
    return reinterpret_cast<float*>(m_ports[port]);
  }

  
  /** This array holds pointers to the port buffers. */
  void* m_ports[kNumParams + 3];
  
  /** The main notemaster object. */
  notemaster n1;
  
  /** Click coefficients for the different sections. */
  // XXX why is this 16 elements long instead of just 3?
  float click[16];
  
  /** The rate the plugin is running at. */
  float samplerate;
  
  /** A factor used to scale rate-dependent values. */
  float rate_scale;
  
  /** Frame counter. */
  long samplecount;
  
  /** Keyboard split point. */
  long splitpoint;
  
  /** True for the parameters that need to be sent to the worker thread for
      non-RT safe processing, false for all others. */
  bool slow_controls[kNumParams];
  
  /** Stores the last value for every parameter. */
  // XXX maybe we could use m_value[i].new_value for this instead?
  float last_value[kNumParams];
  
  /** The master waveform. */
  float tonewheel[WAVETABLESIZE];
  
  /** Resized waveforms at different pitches, one for each drawbar. */
  float sin_16[WAVETABLESIZE];
  float sin_8[WAVETABLESIZE];
  float sin_513[WAVETABLESIZE];
  float sin_4[WAVETABLESIZE];
  float sin_223[WAVETABLESIZE];
  float sin_2[WAVETABLESIZE];
  float sin_135[WAVETABLESIZE];
  float sin_113[WAVETABLESIZE];
  float sin_1[WAVETABLESIZE];

  // TABLES_PER_CHANNEL tables per channel; 3 channels; 1 spare table
#define TABLES_PER_CHANNEL 8
  volatile float wavetable[WAVETABLESIZE*TABLES_PER_CHANNEL*3+1]; 

  lfo  vlfo;
  delay vdelay1, vdelay2;
  float viblfo;
  bool vibchanged1, vibchanged2;
  float vmix1, vmix2;
  filt_lp warmth;

  filt1 fuzz_filt, body_filt, postbody_filt;
  float fuzz;
  float oldmrvalve;
  bool odchanged;
  float oldmix;
  float odmix, n_odmix, n2_odmix, n25_odmix, odmix75;

  float oldspread, spread, spread2;
  float cross1;
  float lspeed, uspeed;
  float er_r, er_r_before, er_l, er_feedback;
  float llfo_out, llfo_nout, llfo_d_out, llfo_d_nout;
  float lfo_out, lfo_nout, lfo_d_out, lfo_d_nout;
  bool lfos_ok;
  filt1 split;
  filt1 horn_filt, damp;
  delay wand_r, wand_l, delay1, delay2, delay3, delay4;
  lfo  lfo1, lfo2, lfo3, lfo4;

  int   last_shape;
  float  last_r, last_l;

  filt_allpass allpass_l[4], allpass_r[4];
 
  bool pedal;
 
  unsigned char* midi_ptr;
 
  pthread_mutex_t m_notemaster_lock;
  
  /** This structure is used to pass data from the audio thread to the
      worker thread. The audio thread trywaits for the semaphore, and if it
      succeeds it writes the new port values to the new_value fields and then
      posts the semaphore. The worker thread waits for the semaphore, copies
      new_value to old_value, posts the semaphore, and does things with
      old_value. */
  struct ParameterChange {
    float old_value;
    volatile float new_value;
  } m_values[kNumParams];
  sem_t m_qsem;
  
  pthread_t m_worker;
  
};


#endif
