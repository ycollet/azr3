/****************************************************************************
    
    AZR-3 - An LV2 synth plugin
    
    Copyright (C) 2006-2007  Lars Luthman <lars.luthman@gmail.com>
    
    based on source code from the VST plugin AZR-3, (C) 2006 Philipp Mott
    (well, almost all of it is his code)
    
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

/*
  This is a mixture of simple hacks and sophisticated effect algorithms.
  There's probably a lot of optimization potential in here...
*/
#include "fx.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#ifdef WIN32
#include <wtypes.h>
#endif


delay::delay(int buflen, bool interpolate)
{
  offset = 0;
  samplerate = 44100;
  int     y;
  p_buflen=buflen;
  interp=interpolate;
  buffer=new float[p_buflen];
  for(y=0;y<p_buflen;y++)
    buffer[y]=0;
  lastIn=0;
  readp=0;
  writep=p_buflen>>1;
}


void delay::set_delay(float dtime)
{
  offset=dtime*samplerate*.001f;

  if(offset<0.1f)
    offset=0.1f;
  else if(offset>=p_buflen)
    offset=(float)p_buflen-1;

  outPointer=writep-offset;
  if(outPointer<0)
    outPointer+=p_buflen;
        
  readp=(int)outPointer;
  alpha=outPointer-readp;
  alpha2=alpha*alpha;
  alpha3=alpha2*alpha;
}

void delay::set_samplerate(float sr)
{
  set_delay(1000*offset/samplerate);
  samplerate=sr;
}

void delay::flood(float value)
{
  int x;
  for(x=0;x<p_buflen;x++)
    buffer[x]=value;
}

delay::~delay()
{
  delete[] buffer;
}

float delay::clock(float input)
{
  float   output=0;
  int             ym1p,y1p,y2p;
  if(p_buflen>4410)
    {
      return(0);
    }

  buffer[writep]=input;

  if(interp)
    {
      ym1p=readp-1;
      if(ym1p<0)
	ym1p+=p_buflen;
      y1p=readp+1;
      if(y1p>=p_buflen)
	y1p-=p_buflen;
      y2p=readp+2;
      if(y2p>=p_buflen)
	y2p-=p_buflen;
                
                
      ym1=buffer[ym1p];
      y0=buffer[readp];
      y1=buffer[y1p];
      y2=buffer[y2p];
                
      output=(alpha3*(y0-y1+y2-ym1)+
	      alpha2*(-2*y0+y1-y2+2*ym1)+
	      alpha*(y1-ym1)+y0);
    }
  else
    output=buffer[readp];

  writep++;
  if(writep>=p_buflen)
    writep-=p_buflen;
  readp++;
  if(readp>=p_buflen)
    readp-=p_buflen;

  return(output);
}

lfo::lfo(float sr)
{
  // XXX make this initialise itself properly
  output=0;
  inc=0;
  dir=1;
  c=1;
  s=0;
  samplerate = sr;
  set_rate(0, 0);
}

void lfo::set_samplerate(float sr)
{
  samplerate=sr;
}

void lfo::set_rate(float srate,int type)
{
  my_srate=srate;
  my_type=type;
  if(type==0)
    inc=2.0f*PI*srate/samplerate;
  else
    inc=2*srate/samplerate;
  ci=cosf(inc);
  si=sinf(inc);
}

float lfo::get_rate()
{
  return(my_srate);
}

void lfo::set_phase(float phase)
{
  if(phase>=0 && phase <=1)
    {
      output=phase;
      s=phase;
    }
}

float lfo::clock()
{
  if(my_type==1)                  // triangle wave
    {
      if(dir==1)
	output+=inc;
      else
	output-=inc;
                
      if(output>=1)
	{
	  dir=-1;
	  output=1;
	}
      else if(output<=0)
	{
	  dir=1;
	  output=0;
	}
    }
  else if(my_type==0)     // sine wave
    {
      nc=DENORMALIZE(c*ci-s*si);
      ns=DENORMALIZE(c*si+s*ci);
      c=nc;
      s=ns;
      output=(s+1)/2;
    }

  return(output);
}

void lfo::offset_phase(lfo& l, float phase_offset) {
  c = l.c;
  s = l.s;
  float tci = cosf(phase_offset);
  float tsi = sinf(phase_offset);
  nc = DENORMALIZE(c * tci - s * tsi);
  ns = DENORMALIZE(c * tsi + s * tci);
  c = nc;
  s = ns;
}
