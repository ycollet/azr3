/****************************************************************************
    
    distortion.hpp - A distortion effect
    
    Copyright (C) 2007  Lars Luthman <lars.luthman@gmail.com>

    These filters are based on source code from the VST plugin 
    AZR-3, (C) 2006 Philipp Mott
        

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as 
    published by the Free Software Foundation.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301  USA

****************************************************************************/

#ifndef FILTERS_HPP
#define FILTERS_HPP

#include <cmath>

#ifndef DENORMALIZE
#define DENORMALIZE(fv) (fv<.00000001f && fv>-.00000001f)?0:(fv)
#endif
#ifndef PI
#define	PI	3.14159265358979323846f
#endif


class filt1 {
  friend class filt_lp;
public:
  inline filt1();
  inline ~filt1(){};
  inline float lp();
  inline float bp();
  inline float hp();
  inline void clock(float input);
  inline void setparam(float cutoff, float q, float samplerate);
  inline void set_samplerate(float samplerate);
private:
  float fs;		// sampling freq
  float fc;		// cutoff freq
  float q;		// resonance
  float m_l;	// lp out
  float m_h;	// hp out
  float m_b;	// bp out
  float m_f,m_q,m_qnrm;
};

class filt_lp : public filt1 {
public:
  inline filt_lp();
  inline float clock(float input);
};
  
class filt_allpass {
public:
  inline filt_allpass() : a1(0.f), zm1(0.f) {
    a1=zm1=my_delay=y=0;
  }
  
  inline void reset() {
    a1=zm1=y=0;
    set_delay(my_delay);
  }
  
  inline void set_delay(float delay) {
    my_delay=delay;
    a1=(1-delay)/(1+delay);
    a1=DENORMALIZE(a1);
  }
  
  inline float clock(float input) {
    if(input<.00000001f && input>-.00000001f)	// prevent Pentium FPU Normalizing
      return(0);
    
    y=-a1*input + zm1;
    zm1=y*a1+input;
    return(y);
  }
private:
  float a1,zm1,my_delay,y;
};


filt1::filt1()
  : q(0),
    m_l(0),
    m_h(0),
    m_b(0),
    m_f(0) {
  
}


void filt1::clock(float input) {
  float in;
  in = DENORMALIZE(input);
  m_l = DENORMALIZE(m_l);
  m_b = DENORMALIZE(m_b);
  
  m_h = in - m_l - q * m_b;
  m_b += m_f * m_h;
  m_l += m_f * m_b;
}


float filt1::lp() {
  return m_l;
}


float filt1::bp() {
  return m_b;
}


float filt1::hp() {
  return m_h;
}


void filt1::set_samplerate(float samplerate) {
  fs = samplerate;
  m_l = m_h = m_b = 0;
  setparam(fc, q, fs);
}


void filt1::setparam(float cutoff, float mq, float samplerate) {
  fc = cutoff;
  q = mq;
  fs = samplerate;
  m_f = 2.0f * sinf(PI * fc / fs);
}


filt_lp::filt_lp() 
  : filt1() {
  
}


float filt_lp::clock(float input) {
  float in;
  in = DENORMALIZE(input);
  m_l = DENORMALIZE(m_l);
  m_b = DENORMALIZE(m_b);
  
  m_h = in - m_l - q * m_b;
  m_b += m_f * m_h;
  m_l += m_f * m_b;
  
  return m_l;
}


#endif
