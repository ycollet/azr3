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

#ifndef __Voice_Classes_h__
#define __Voice_Classes_h__

#include "fx.hpp"
#include "filters.hpp"

#define MAXVOICES	32

// voice stati	(status)

#define	VS_IDLE		0
#define	VS_PLAYING	1
#define	VS_WAIT_PUP	2	// has note_off and waits for pedal up

// VCA phases	(vca_phase)

#define	VP_IDLE		0
#define	VP_A		1	// attack
#define VP_D		2	// decay
#define	VP_S		3	// sustain = main phase
#define	VP_R		4	// release
#define	VP_FR		5	// fast release
#define	VP_FA		6	// fast attack

#include <stdio.h>

char*	note2str(long note);


/** This is a single organ voice. */
class voice {
 public:
  voice();
  ~voice() {}
  float	clock();
  void	reset();
  void	suspend();
  void	resume();
  void	note_on(long note, long velocity, volatile float *table, int size, float pitch, bool percenable, float sclick, float sust);
  void	note_off(long note);
  void	force_off();
  long	get_note();
  bool	check_note(long note);
  bool	get_active();
  void	set_samplerate(float samplerate);
  void	set_attack(float attack);
  void	set_decay(float decay);
  void	set_release(float release);
  void	set_pedal(bool pedal);
  void	set_percussion(float percussion,float perc_multiplier,float percfade);
  void	set_pitch(float pitch);
  void	voicecalc();
	
 private:
  unsigned char	samplecount1,samplecount2;
  int		status;
  int		iphase;
  float	y0,y1,fract;
  float	samplerate_scaler;	// Anpassung der Wavetable-Logik an Samplerate
  float	phase;				// Position in der Wavetable
  float	phaseinc;			// increment for phase
  float	output;				// Ausgang
  float	click;				// click strength
  float	hertz,a,s0,s1;		// percussion sine values
  float	percmultiplier;		// percussion octave multiplier
  float	perc;				// percussion volume
  bool	perc_ok;
  float	perc_decay;
  float	perc_vca;
  int		perc_phase;
  float	perc_attack;
  float	perc_fade;
  long	actual_note;	// Note-Daten
  long	next_note;			// Vorbesetzung von actual_note.
  long	perc_next_note;
  volatile float	*my_table;			// die Wavetable
  long	mask,my_size;		// Maske und Grˆﬂe der Wavetable
  float	samplerate;
  double	midi_scaler;		// Umrechung Midi->float-Faktor [0..1]
  float	freqtab[128];		// Umrechnung Midi->Frequenz
  float	noise;
  float	clickattack;
  float	clickvol;
  float	adsr_attack;
  float	adsr_decay;
  float	adsr_sustain;
  float	adsr_release;
  float	adsr_fast_release;
  float	sustain;
  float	VCA;				// VCA-Faktor. Wird durch attack und release beeinfluﬂt
  int		vca_phase;			// 1:Attack 2:Release
  bool	pedal;				// Pedalzustand
  float	pitch;
  filt1	clicklp;
};


/** This class mixes and manages all voices. */
class notemaster {
 public:
  notemaster(int number);		// Anzahl der Stimmen
  ~notemaster();
  void	set_numofvoices(int number);
  void	note_on(long note, long velocity, volatile float *table, int size1, int channel, bool percenable, float click, float sustain);
  void	all_notes_off();
  float	*clock();
  void	note_off(long note, int channel);
  void	set_pedal(int pedal, int channel);
  void	set_percussion(float percussion,float perc_multiplier,float percfade);
  void	set_samplerate(float samplerate);
  void	set_pitch(float pitch, int channel);
  void	set_volume(float vol, int channel);
  void	reset();
  void	suspend();
  void	resume();
 private:
  voice	*voices[MAXVOICES+1];
  int		numofvoices;
  unsigned long	age[MAXVOICES];
  unsigned char	chan[MAXVOICES];
  float	volume[MAXVOICES];
  float	output[16];
  int		x;
  float	pitch,next_pitch;

  float	my_click,my_percussion,my_perc_multiplier,my_percfade,my_samplerate;
};

#endif
