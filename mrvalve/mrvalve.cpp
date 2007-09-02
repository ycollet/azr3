/****************************************************************************
    
    Mr Valve - LV2 distortion effect
    
    Copyright (C) 2007 Lars Luthman <lars.luthman@gmail.com>
    
    based on source code from the VST plugin AZR-3, (C) 2006 Philipp Mott
    
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

#include <cstring>
#include <iostream>

#include "lv2.h"
#include "distortion.hpp"
#include "mrvalve.peg"


using namespace std;


static inline float clamp(float v, float l, float u) {
  v = v < l ? l : v;
  v = v > u ? u : v;
  return v;
}


/** This is the class that contains all the code and data for the audio
    identity plugin. */
class MrValve {
public:
  
  MrValve(double rate, const char*, const LV2_Host_Feature* const*) 
    : m_dist(uint32_t(rate)) {
    
  }
  
  
  void connect_port(uint32_t index, void* buffer) {
    m_ports[index] = buffer;
  }
  
  
  void run(uint32_t nframes) {
    float drive = clamp(*p(v_drive), 0, 1);
    float set = clamp(*p(v_set), 0, 1);
    float tone = clamp(*p(v_tone), 0, 1);
    float mix = clamp(*p(v_mix), 0, 1);
    
    if (p(v_input) != p(v_output))
      std::memcpy(p(v_output), p(v_input), nframes * sizeof(float));
    
    m_dist.run(p(v_output), nframes, 1, drive, set, tone, mix);
  }
  
protected:
  
  /** Use this as a shorthand to access and cast port buffers. */
  inline float* p(uint32_t port) {
    return reinterpret_cast<float*>(m_ports[port]);
  }

  Distortion m_dist;
  void* m_ports[6];
  
};


/* The LV2 interface. */

namespace {
  
  LV2_Handle instantiate(const LV2_Descriptor* desc,
			 double frame_rate,
			 const char* bundle_path,
			 const LV2_Host_Feature* const* features) {
    return new MrValve(frame_rate, bundle_path, features);
  }
  
  
  void connect_port(LV2_Handle instance, uint32_t port, void* buffer) {
    static_cast<MrValve*>(instance)->connect_port(port, buffer);
  }
  
  
  void run(LV2_Handle instance, uint32_t frames) {
    static_cast<MrValve*>(instance)->run(frames);
  }
  
  
  void cleanup(LV2_Handle instance) {
    delete static_cast<MrValve*>(instance);
  }
  
}


extern "C" {
  
  const LV2_Descriptor *lv2_descriptor(uint32_t index) {
    static LV2_Descriptor d = {
      strdup("http://ll-plugins.nongnu.org/lv2/dev/mrvalve/0.0.0"),
      &instantiate,
      &connect_port,
      0,
      &run,
      0,
      &cleanup,
      0
    };
    if (index == 0)
      return &d;
    return 0;
  }

}
