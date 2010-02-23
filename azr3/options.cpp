/****************************************************************************
    
    AZR-3 - An organ synth
    
    Copyright (C) 2006-2010 Lars Luthman <lars.luthman@gmail.com>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as published
    by the Free Software Foundation.

    This particular file can also be redistributed and/or modified under the
    terms of the GNU General Public License version 3 or later.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA

****************************************************************************/
#include "options.hpp"

#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <stdexcept>


using namespace std;


OptionValueBase::OptionValueBase(string const& long_n, string const& short_n, 
				 string const& value_n, string const& doc) 
  : long_name(long_n),
    short_name(short_n),
    value_name(value_n),
    doc_str(doc),
    bare(false) { 
}


OptionParser::~OptionParser() {
  map<string, OptionValueBase*>::iterator i;
  for (i = m_long_opts.begin(); i != m_long_opts.end(); ++i)
    delete i->second;
}
  

OptionParser& OptionParser::add(string const& long_opt, 
				string const& short_opt, 
				bool& data, string const& doc = "") {
  return add(long_opt, short_opt, "true|false", data, doc);
}
 

OptionParser& OptionParser::add_bare(string const& long_opt, 
				     string const& short_opt, 
				     bool& data, string const& doc = "")  {
  OptionValue<bool>* oval = new OptionValue<bool>(long_opt, short_opt, 
						  "", data, doc);
  struct leak_protector {
    leak_protector(OptionValue<bool>* p) : cool(false), ptr(p) { }
    ~leak_protector() { if (!cool) delete ptr; }
    bool cool;
    OptionValue<bool>* ptr;
  } lp(oval);
  
  oval->bare = true;
  m_long_opts[long_opt] = oval;
  m_short_opts[short_opt] = oval;
  
  lp.cool = true;
  return *this;
}


OptionParser& OptionParser::set_env_prefix(string const& prefix) {
  m_env_prefix = prefix;
  return *this;
}
 

OptionParser& OptionParser::parse(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      map<string, OptionValueBase*>::iterator iter;
      
      // long option
      if (argv[i][1] == '-') {
	
	// find the equality sign and extract the name
	int ep;
	for (ep = 2; argv[i][ep] != '\0' && argv[i][ep] != '='; ++ep);
	string opt_name(&argv[i][2], ep - 2);
	
	// look up the name and set the value
	iter = m_long_opts.find(opt_name);
	if (iter == m_long_opts.end()) {
	  throw runtime_error(string("Unknown program option: ") + opt_name);
	}
	else if (iter->second->bare) {
	  if (argv[i][ep] == '=') {
	    throw runtime_error(string("Program option ") + opt_name + 
				" must not have a value.");
	  }
	  iter->second->set("true");
	}
	else {
	  if (argv[i][ep] == '\0') {
	    throw runtime_error(string("Program option ") + opt_name +
				" must have a value.");
	  }
	  iter->second->set(&argv[i][ep + 1]);
	}
      }
      
      // short option
      else {
	
	// extract the name
	string opt_name(&argv[i][1]);
	
	// look up the name and set the value
	iter = m_short_opts.find(opt_name);
	if (iter == m_short_opts.end()) {
	  throw runtime_error(string("Unknown program option: ") + opt_name);
	}
	else if (iter->second->bare) {
	  iter->second->set("true");
	}
	else {
	  if (argc == i + 1) {
	    throw runtime_error(string("Program option ") + opt_name +
				" must have a value.");
	  }
	  else {
	    iter->second->set(argv[i + 1]);
	  }
	  
	  // skip the next argument, it's the value for this one
	  ++i;
	  
	}
      }
    }
    else {
      throw runtime_error(string("Unknown program option: ") + argv[i]);
    }
  }
  
  return *this;
}
  

OptionParser& OptionParser::parse_env() {
  map<string, OptionValueBase*>::iterator iter;
  for (iter = m_long_opts.begin(); iter != m_long_opts.end(); ++iter) {
    string env_name(iter->first);
    for (string::iterator si = env_name.begin(); si != env_name.end(); ++si) {
      if (*si == '-')
	*si = '_';
      else if (isalpha(*si))
	*si = toupper(*si);
    }
    env_name = m_env_prefix + env_name;
    char* env = getenv(env_name.c_str());
    if (env) {
      if (iter->second->bare)
	iter->second->set("true");
      else
	iter->second->set(env);
    }
  }
  return *this;
}
  
  
void OptionParser::print_help(ostream& os) {
  map<string, OptionValueBase*>::const_iterator iter;
  for (iter = m_long_opts.begin(); iter != m_long_opts.end(); ++iter) {
    OptionValueBase& ovb = *iter->second;
    string line(" -");
    line += ovb.short_name + ", --" + ovb.long_name;
    if (!ovb.bare)
      line += string("=") + ovb.value_name;
    os<<line;
    if (line.size() > 28) {
      os<<'\n';
      os<<setfill(' ')<<setw(29)<<' ';
    }
    else {
      os<<setfill(' ')<<setw(29 - line.size())<<' ';
    }
    for (size_t p = 0; p < string::npos; ) {
      size_t np = ovb.doc_str.find('\n', p);
      if (np < ovb.doc_str.size()) {
	os<<ovb.doc_str.substr(p, np - p + 1)<<setfill(' ')<<setw(29)<<' ';
	p = np + 1;
      }
      else {
	os<<ovb.doc_str.substr(p);
	p = np;
      }
    }
    os<<'\n';
  }
  os<<flush;
}
  
  
void OptionParser::print_env_help(ostream& os) {
  os<<"\nAll program options can also be set using environment variables.\n"
    <<"The variable names are the same as the long option names with all\n"
    <<"letters in upper case, all '-' changed to '_' and the prefix\n"
    <<"'"<<m_env_prefix<<"' added.\n\n"
    <<"Command line options override environment variables."<<endl;
}


