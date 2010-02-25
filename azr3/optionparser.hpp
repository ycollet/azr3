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
#ifndef OPTIONPARSER_HPP
#define OPTIONPARSER_HPP

#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>


/** The base class for program option objects. It stores the long and short
    names of the option, the value name, the documentation text and the
    reference to the actual object that should be modified when this option
    is used. 
    @internal
*/
class OptionValueBase {
public:
  OptionValueBase(std::string const& long_n, std::string const& short_n, 
		  std::string const& value_n, std::string const& doc);
  
  /** This function is called by the parser when the user has set this option
      from the command line or via environment variables. */
  virtual void set(std::string const& str) = 0;
  
  virtual ~OptionValueBase() { }

  std::string long_name;
  std::string short_name;
  std::string value_name;
  std::string doc_str;
  bool bare;
};


/** A class template for generic program option objects. The datatype must
    be readable from an istream. 
    @internal
*/
template <typename T>
class OptionValue : public OptionValueBase {
public:

  OptionValue(std::string const& long_n, std::string const& short_n, 
	      std::string const& value_n, T& d, std::string const& doc) 
    : OptionValueBase(long_n, short_n, value_n, doc),
      data(d) { 
  }

  void set(std::string const& str) {
    std::istringstream iss(str);
    iss>>data;
    if (iss.fail()) {
      throw std::runtime_error(std::string("Failed to set the value of ") + 
			       long_name);
    }
  }
  
  T& data;
};


/** We need a specialisation for strings since they are read one word at
    a time from streams. 
    @internal
*/
template <>
class OptionValue<std::string> : public OptionValueBase {
public:
  
  OptionValue(std::string const& long_n, std::string const& short_n, 
	      std::string const& value_n, std::string& d, 
	      std::string const& doc) 
    : OptionValueBase(long_n, short_n, value_n, doc),
      data(d) { 
  }
  
  void set(std::string const& str) {
    data = str;
  }
  
  std::string& data;
};


/** We need a specialisation for bools so we can read 'true' and 'false'. 
    @internal
*/
template <>
class OptionValue<bool> : public OptionValueBase {
public:
  
  OptionValue(std::string const& long_n, std::string const& short_n, 
	      std::string const& value_n, bool& d, std::string const& doc)
    : OptionValueBase(long_n, short_n, value_n, doc),
      data(d) { 
  }
  
  void set(std::string const& str) {
    if (str == "true")
      data = true;
    else if (str == "false")
      data = false;
    else
      throw std::runtime_error(std::string("Invalid value for program option ")+
			       long_name + ".");
  }
  
  bool& data;
};


/** This is a parser that extracts user-set program options. You can add 
    variables using add() and add_bare(), parse command line arguments
    and environment variables using parse() and parse_env() and print a
    descriptive help text using print_help() and print_env_help().
*/
class OptionParser {
public:
  
  OptionParser() { }
  
  ~OptionParser();
  
  /** Add a new program option.
      @param long_opt The long option name.
      @param short_opt The short option name.
      @param value_name The name that will be used in place of the option 
                        value in the help text.
      @param data A reference to a data object that will be set by the option.
                  it needs to be readable from an istream.
      @param doc A short string that describes the option. If it's longer than
                 50 characters linebreaks should be added.
  */
  template <typename T>
  OptionParser& add(std::string const& long_opt, std::string const& short_opt, 
		    std::string const& value_name, T& data, 
		    std::string const& doc) {
    OptionValue<T>* oval = new OptionValue<T>(long_opt, short_opt, 
					      value_name, data, doc);
    m_long_opts[long_opt] = oval;
    m_short_opts[short_opt] = oval;
    return *this;
  }
  
  /** Specialised overload of add() for @c bool options. No value name is
      needed, the string "true|false" will be used. */
  OptionParser& add(std::string const& long_opt, std::string const& short_opt, 
		    bool& data, std::string const& doc);
  
  /** Add an option that the user doesn't specify a value for. The bool
      will be set to true if the option is given. */
  OptionParser& add_bare(std::string const& long_opt, 
			 std::string const& short_opt, 
			 bool& data, std::string const& doc);
  
  /** Set a prefix that will be added to the long option names to get the
      corresponding environment variable name. It is recommended to use
      upper case letters. */
  OptionParser& set_env_prefix(std::string const& prefix);
  
  /** Parse the given command line arguments. Any values from old parsings
      will be overwritten. 
  
      This function throws a @c runtime_error if the parsing fails. */
  OptionParser& parse(int argc, char** argv);
  
  /** Parse the environment. Any values from old parsings will be 
      overwritten. */
  OptionParser& parse_env();
  
  /** Print help text for all the options. */
  void print_help(std::ostream& os);
  
  /** Print an explanation of the environment variables. */
  void print_env_help(std::ostream& os);
  
private:
  
  OptionParser(OptionParser const&) { };
  OptionParser& operator=(OptionParser const&) { return *this; };
  
  std::map<std::string, OptionValueBase*> m_long_opts;
  std::map<std::string, OptionValueBase*> m_short_opts;
  std::string m_env_prefix;
};


#endif
