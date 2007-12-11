/****************************************************************************
    
    AZR-3 - An organ synth
    
    Copyright (C) 2006 Lars Luthman <lars.luthman@gmail.com>
    
    based on source code from the VST plugin AZR-3, (C) 2006 Philipp Mott
    (well, almost all of it is his code)
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA

****************************************************************************/

#ifndef	__globals_h__
#define __globals_h__

#define	VERSION	"1.3"

#define	WAVETABLESIZE	200

const float Pi = 3.14159265358979323f;

#define	NUMOFVOICES		11

//#define _fabsf(fv) (fv<0)?(-fv):(fv)
#define _fabsf(fv) fabsf(fv)

/*
This cute little trick helps to prevent the infamous Pentium
normalization effect.
Use this macro whenever you have an feedback float variable
which is expected to return decreasingly small values.
*/
#define DENORMAL(fv) (_fabsf(fv)<.000001f)?0:(fv)


/*
	ID and Effect name are used by the host application to distinguish
	between plugins. You can register a specific ID through a Steinberg
	service.
*/
#define MY_ID			'FLP5'
#define	EFFECT_NAME		"AZR3"
#define	VENDOR_STRING	"Rumpelrausch Täips"
#define	PRODUCT_STRING	"AZR3"

#define	LAST_PARAM		n_3_sustain

enum
{
	W_SINE=0,
	W_SINE1,
	W_SINE2,
	W_SINE3,
	W_TRI,
	W_SAW,
	W_NUMOF
};

enum	// bitmap indices
{
	b_panelfx=0,
	b_panelvoice,
	b_cknob,
	b_dbblack,
	b_dbwhite,
	b_dbbrown,
	b_inside,
	b_miniledred,
	b_minioffon,
	b_onoffgreen,
	b_vonoff,
	b_yellow,
	b_vu,
	kNumBitmaps
};

enum
{
	kNumPrograms = 32,
	kNumOutputs = 2,
// The parameters
// The order shows how the code has grown in time...
	n_mono=0,
	n_click,
	n_bender,
	n_shape,
	n_perc,
	n_percvol,
	n_percfade,
	n_vol1,
	n_vol2,
	n_vol3,
	n_master,
//11
	n_1_perc,
	n_1_db1,
	n_1_db2,
	n_1_db3,
	n_1_db4,
	n_1_db5,
	n_1_db6,
	n_1_db7,
	n_1_db8,
	n_1_db9,
//21
	n_1_vibrato,
	n_1_vstrength,
	n_1_vmix,
	n_2_perc,
	n_2_db1,
	n_2_db2,
	n_2_db3,
	n_2_db4,
	n_2_db5,
	n_2_db6,
//31
	n_2_db7,
	n_2_db8,
	n_2_db9,
	n_2_vibrato,
	n_2_vstrength,
	n_2_vmix,
	n_3_perc,
	n_3_db1,
	n_3_db2,
	n_3_db3,
//41
	n_3_db4,
	n_3_db5,
	n_mrvalve,
	n_drive,
	n_set,
	n_tone,
	n_mix,
	n_speakers,
	n_speed,
	n_l_slow,
//51
	n_l_fast,
	n_u_slow,
	n_u_fast,
	n_belt,
	n_spread,
	n_complex,
	n_pedalspeed,
	n_splitpoint,
	n_sustain,
	n_1_sustain,
	n_2_sustain,
	n_3_sustain,

	kNumParams,

	n_1_midi,
	n_2_midi,
	n_3_midi,
	n_vu,
	n_compare,
	n_save,
	n_display,
	n_redraw,
	n_setprogname,
	n_param_is_manual,
	n_splash,
	n_voicemode,
	n_fxmode,
	n_split,
	n_output,
	kNumControls,
	n_mute
};


#endif
