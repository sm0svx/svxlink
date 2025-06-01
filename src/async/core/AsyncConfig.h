/**
@file	 AsyncConfig.h
@brief   A class for reading "INI-foramtted" configuration files
@author  Tobias Blomberg
@date	 2004-03-17

This file contains a class that is used to read configuration files that is
in the famous MS Windows INI file format. An example of a configuration file
is shown below.

\include test.cfg

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2025 Tobias Blomberg / SM0SVX

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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/

/** @example AsyncConfig_demo.cpp
An example of how to use the Config class
*/


#ifndef ASYNC_CONFIG_INCLUDED
#define ASYNC_CONFIG_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdio.h>
#include <sigc++/sigc++.h>

#include <string>
#include <map>
#include <list>
#include <memory>
#include <sstream>
#include <locale>
#include <vector>
#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

  

/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief	A class for reading INI-formatted configuration files
@author Tobias Blomberg
@date   2004-03-17

This class is used to read configuration files that is in the famous MS Windows
INI file format. An example of a configuration file and how to use the class
is shown below.

\include test.cfg

\include AsyncConfig_demo.cpp
*/
class Config
{
  public:
    /**
     * @brief 	Default constuctor
     */
    Config(void) {}
  
    /**
     * @brief 	Destructor
     */
    ~Config(void);
  
    /**
     * @brief 	Open the given config file
     * @param 	name The name of the configuration file to open
     * @return	Returns \em true on success or else \em false.
     *
     * This function will read the given configuration file into memory.
     * If this function return false and errno != 0, the errno variable may
     * give a hint what the problem was.
     */
    bool open(const std::string& name);
    
    /**
     * @brief 	Return the string value of the given configuration variable
     * @param 	section The name of the section where the configuration
     *	      	      	variable is located
     * @param 	tag   	The name of the configuration variable to get
     * @return	Returns String with content of the configuration variable.
     *                  If no variable is found an empty string is returned
     *
     * This function will return the string value corresponding to the given
     * configuration variable. If the configuration variable is unset, an
     * empty sting is returned.
     */
    const std::string &getValue(const std::string& section,
				 const std::string& tag) const;

    /**
     * @brief 	Get the string value of the given configuration variable
     * @param 	section    The name of the section where the configuration
     *	      	      	   variable is located
     * @param 	tag   	   The name of the configuration variable to get
     * @param 	value 	   The value is returned in this argument. Any previous
     *	      	      	   contents is wiped
     * @return	Returns \em true on success or else \em false on failure
     *
     * This function is used to get the value for a configuration variable
     * of type "string".
     */
    bool getValue(const std::string& section, const std::string& tag,
                  std::string& value, bool missing_ok = false) const;

    /**
     * @brief 	Get the char value of the given configuration variable
     * @param 	section    The name of the section where the configuration
     *	      	      	   variable is located
     * @param 	tag   	   The name of the configuration variable to get
     * @param 	value 	   The value is returned in this argument. Any previous
     *	      	      	   contents is wiped
     * @return	Returns \em true on success or else \em false on failure
     *
     * This function is used to get the value for a configuration variable
     * of type "char". It is an error if the size of the value is anything but
     * 1 byte.
     */
    bool getValue(const std::string& section, const std::string& tag,
                  char& value, bool missing_ok = false) const;
    /**
     * @brief 	Get the value of the given configuration variable.
     * @param 	section    The name of the section where the configuration
     *	      	      	   variable is located
     * @param 	tag   	   The name of the configuration variable to get
     * @param 	rsp 	   The value is returned in this argument.
     *	      	      	   Successful completion overwrites previous contents
     * @param	missing_ok If set to \em true, return \em true if the
     *                     configuration variable is missing
     * @return	Returns \em true on success or else \em false on failure.
     *
     * This function is used to get the value of a configuraiton variable.
     * It's a template function meaning that it can take any value type
     * that supports the operator>> function. Note that when the value is of
     * type string, the overloaded getValue is used rather than this function.
     * Normally a missing configuration variable is seen as an error and the
     * function returns \em false. If the missing_ok parameter is set to
     * \em true, this function returns \em true for a missing variable but
     * still returns \em false if an illegal value is specified.
     */
    template <typename Rsp>
    bool getValue(const std::string& section, const std::string& tag,
		  Rsp &rsp, bool missing_ok = false) const
    {
      std::string str_val;
      if (!getValue(section, tag, str_val))
      {
	return missing_ok;
      }
      std::stringstream ssval(str_val);
      Rsp tmp;
      ssval >> tmp;
      if(!ssval.eof())
      {
        ssval >> std::ws;
      }
      if (ssval.fail() || !ssval.eof())
      {
	return false;
      }
      rsp = tmp;
      return true;
    } /* Config::getValue */

    /**
     * @brief 	Get the value of the given config variable into container
     * @param 	section    The name of the section where the configuration
     *	      	      	   variable is located
     * @param 	tag   	   The name of the configuration variable to get
     * @param 	c 	   The value is returned in this argument.
     *	      	      	   Successful completion overwrites previous contents
     * @param	missing_ok If set to \em true, return \em true if the
     *                     configuration variable is missing
     * @return	Returns \em true on success or else \em false on failure.
     *
     * This function is used to get the value of a configuraiton variable.
     * The config variable is read into a container (e.g. vector, list etc).
     * It's a template function meaning that it can take any value type
     * that supports the operator>> function. 
     * Normally a missing configuration variable is seen as an error and the
     * function returns \em false. If the missing_ok parameter is set to
     * \em true, this function returns \em true for a missing variable but
     * still returns \em false if an illegal value is specified.
     */
    template <template <typename, typename> class Container,
              typename Value>
    bool getValue(const std::string& section, const std::string& tag,
		  Container<Value, std::allocator<Value> > &c,
                  bool missing_ok = false) const
    {
      std::string str_val;
      if (!getValue(section, tag, str_val))
      {
	return missing_ok;
      }
      if (str_val.empty())
      {
        c.clear();
        return true;
      }
      std::stringstream ssval(str_val);
      ssval.imbue(std::locale(ssval.getloc(), new csv_whitespace));
      while (!ssval.eof())
      {
        Value tmp;
        ssval >> tmp;
        if(!ssval.eof())
        {
          ssval >> std::ws;
        }
        if (ssval.fail())
        {
          return false;
        }
        c.push_back(tmp);
      }
      return true;
    } /* Config::getValue */

    /**
     * @brief 	Get the value of the given config variable into keyed container
     * @param 	section    The name of the section where the configuration
     *	      	      	   variable is located
     * @param 	tag   	   The name of the configuration variable to get
     * @param 	c 	   The value is returned in this argument.
     *	      	      	   Successful completion overwrites previous contents
     * @param	missing_ok If set to \em true, return \em true if the
     *                     configuration variable is missing
     * @return	Returns \em true on success or else \em false on failure.
     *
     * This function is used to get the value of a configuraiton variable.
     * The config variable is read into a keyed container (e.g. set, multiset
     * etc).
     * It's a template function meaning that it can take any key type
     * that supports the operator>> function.
     * Normally a missing configuration variable is seen as an error and the
     * function returns \em false. If the missing_ok parameter is set to
     * \em true, this function returns \em true for a missing variable but
     * still returns \em false if an illegal value is specified.
     */
    template <template <typename, typename, typename> class Container,
              typename Key>
    bool getValue(const std::string& section, const std::string& tag,
                  Container<Key, std::less<Key>, std::allocator<Key> > &c,
                  bool missing_ok = false) const
    {
      std::string str_val;
      if (!getValue(section, tag, str_val))
      {
        return missing_ok;
      }
      if (str_val.empty())
      {
        c.clear();
        return true;
      }
      std::stringstream ssval(str_val);
      ssval.imbue(std::locale(ssval.getloc(), new csv_whitespace));
      while (!ssval.eof())
      {
        Key tmp;
        ssval >> tmp;
        if(!ssval.eof())
        {
          ssval >> std::ws;
        }
        if (ssval.fail())
        {
          return false;
        }
        c.insert(tmp);
      }
      return true;
    } /* Config::getValue */

    /**
     * @brief   Get value of given config variable into associative container
     * @param   section    The name of the section where the configuration
     *                     variable is located
     * @param   tag        The name of the configuration variable to get
     * @param   c          The value is returned in this argument.
     *                     Successful completion overwrites previous contents
     * @param   sep        The character used to separate key and value
     * @param   missing_ok If set to \em true, return \em true if the
     *                     configuration variable is missing
     * @return  Returns \em true on success or else \em false on failure.
     *
     * This function is used to get the value of a configuraiton variable.  The
     * config variable is read into an associative container (e.g. std::map or
     * std::multimap).  It's a template function meaning that it can take any
     * key and value type that supports the operator>> function.
     * Normally a missing configuration variable is seen as an error and the
     * function returns \em false. If the missing_ok parameter is set to \em
     * true, this function returns \em true for a missing variable but still
     * returns \em false if an illegal value is specified.
     */
    template <template <typename, typename, typename, typename> class Container,
              class Key, class T, class Compare=std::less<Key>,
              class Allocator=std::allocator<std::pair<const Key, T>>>
    bool getValue(const std::string& section, const std::string& tag,
                  Container<Key, T, Compare, Allocator>& c,
                  char sep = ':', bool missing_ok = false) const
    {
      std::string str_val;
      if (!getValue(section, tag, str_val))
      {
        return missing_ok;
      }
      if (str_val.empty())
      {
        c.clear();
        return true;
      }
      std::stringstream ssval(str_val);
      ssval.imbue(std::locale(ssval.getloc(), new csv_whitespace));
      while (!ssval.eof())
      {
        std::string entry;
        ssval >> entry;
        std::string::size_type seppos = entry.find(sep);
        if (seppos == std::string::npos)
        {
          return false;
        }
        std::string keystr(entry.substr(0, seppos));
        std::string valuestr(entry.substr(seppos+1));
        Key key;
        T value;
        if (!setValueFromString(key, keystr) ||
           !setValueFromString(value, valuestr))
        {
          return false;
        }
        if(!ssval.eof())
        {
          ssval >> std::ws;
        }
        if (ssval.fail())
        {
          return false;
        }
        c.insert(std::pair<Key, T>(key, value));
      }
      return true;
    } /* Config::getValue */

    /**
     * @brief 	Get a range checked variable value
     * @param 	section    The name of the section where the configuration
     *	      	      	   variable is located
     * @param 	tag   	   The name of the configuration variable to get.
     * @param 	min   	   Smallest valid value.
     * @param 	max   	   Largest valid value.
     * @param 	rsp 	   The value is returned in this argument.
     *	      	      	   Successful completion overwites prevoius contents.
     * @param	missing_ok If set to \em true, return \em true if the
     *                     configuration variable is missing
     * @return	Returns \em true if value is within range otherwise \em false.
     *
     * This function is used to get the value of the given configuration
     * variable, checking if it is within the given range (min <= value <= max).
     * Requires operators >>, < and > to be defined in the value object.
     * Normally a missing configuration variable is seen as an error and the
     * function returns \em false. If the missing_ok parameter is set to
     * \em true, this function returns \em true for a missing variable but
     * till returns \em false if an illegal value is specified.
     */
    template <typename Rsp>
    bool getValue(const std::string& section, const std::string& tag,
		  const Rsp& min, const Rsp& max, Rsp &rsp,
		  bool missing_ok = false) const
    {
      std::string str_val;
      if (!getValue(section, tag, str_val))
      {
	return missing_ok;
      }
      std::stringstream ssval(str_val);
      Rsp tmp;
      ssval >> tmp;
      if(!ssval.eof())
      {
        ssval >> std::ws;
      }
      if (ssval.fail() || !ssval.eof() || (tmp < min) || (tmp > max))
      {
	return false;
      }
      rsp = tmp;
      return true;
    } /* Config::getValue */

    /**
     * @brief Subscribe to the given configuration variable (char*)
     * @param section The name of the section where the configuration
     *                variable is located
     * @param tag     The name of the configuration variable to get
     * @param def     Default value if the config var does not exist
     * @param func    The function to call when the config var changes
     *
     * This function is used to subscribe to the changes of the specified
     * configuration variable. The given function will be called when the value
     * changes. If the configuration variable is not set, it will be set to the
     * given default value.
     *
     * This version of the function is called when the default value is a C
     * string (char*).
     */
    template <typename F=std::function<void(const char*)>>
    void subscribeValue(const std::string& section, const std::string& tag,
                        const char* def, F func)
    {
      subscribeValue(section, tag, std::string(def),
          [=](const std::string& str_val) -> void
          {
            func(str_val.c_str());
          });
    } /* subscribeValue */

    /**
     * @brief Subscribe to the given configuration variable
     * @param section The name of the section where the configuration
     *                variable is located
     * @param tag     The name of the configuration variable to get
     * @param def     Default value if the config var does not exist
     * @param func    The function to call when the config var changes
     *
     * This function is used to subscribe to the changes of the specified
     * configuration variable. The given function will be called when the value
     * changes. If the configuration variable is not set, it will be set to the
     * given default value.
     *
     * This version of the function is called when the default value is of a
     * non-container type (e.g. std::string, int, bool etc).
     */
    template <typename Rsp, typename F=std::function<void(const Rsp&)>>
    void subscribeValue(const std::string& section, const std::string& tag,
                        const Rsp& def, F func)
    {
      Value& v = getValueP(section, tag, def);
      v.subs.push_back(
          [=](const std::string& str_val) -> void
          {
            std::stringstream ssval(str_val);
            ssval.imbue(std::locale(ssval.getloc(), new empty_ctype));
            Rsp tmp;
            ssval >> tmp;
            func(tmp);
          });
      v.subs.back()(v.val);
    } /* subscribeValue */

    /**
     * @brief Subscribe to the given configuration variable (sequence)
     * @param section The name of the section where the configuration
     *                variable is located
     * @param tag     The name of the configuration variable to get
     * @param def     Default value if the config var does not exist
     * @param func    The function to call when the config var changes
     *
     * This function is used to subscribe to the changes of the specified
     * configuration variable. The given function will be called when the value
     * changes. If the configuration variable is not set, it will be set to the
     * given default value.
     *
     * This version of the function is called when the default value is a
     * sequence container (e.g. std::vector, std::list etc).
     */
    template <template <typename, typename> class Container,
              typename Rsp, typename F=std::function<void(const Rsp&)>>
    void subscribeValue(const std::string& section, const std::string& tag,
                        const Container<Rsp, std::allocator<Rsp>>& def, F func)
    {
      Value& v = getValueP(section, tag, def);
      v.subs.push_back(
          [=](const std::string& str_val) -> void
          {
            std::stringstream ssval(str_val);
            ssval.imbue(std::locale(ssval.getloc(), new csv_whitespace));
            Container<Rsp, std::allocator<Rsp>> c;
            while (!ssval.eof())
            {
              Rsp tmp;
              ssval >> tmp;
              if(!ssval.eof())
              {
                ssval >> std::ws;
              }
              if (ssval.fail())
              {
                return;
              }
              c.push_back(tmp);
            }
            func(std::move(c));
          });
      v.subs.back()(v.val);
    } /* Config::subscribeValue */

    /**
     * @brief   Return the name of all configuration sections
     * @return  Returns a list of all existing section names
     */
    std::list<std::string> listSections(void);

    /**
     * @brief 	Return the name of all the tags in the given section
     * @param 	section The name of the section where the configuration
     *	      	      	variables are located
     * @return	Returns the list of tags in the given section
     */
    std::list<std::string> listSection(const std::string& section);

    /**
     * @brief   Set the value of a configuration variable
     * @param 	section   The name of the section where the configuration
     *	      	      	  variable is located
     * @param 	tag   	  The name of the configuration variable to set.
     * @param   value     The value to set
     *
     * This function is used to set the value of a configuration variable.
     * If the given configuration section or variable does not exist, it
     * is created.
     * Note that this function will not write anything back to the
     * associated configuration file. It will only set the value in memory.
     *
     * The valueUpdated signal will be emitted so that subscribers can get
     * notified when the value of a configuration variable is changed.
     */
    void setValue(const std::string& section, const std::string& tag,
      	      	  const std::string& value);

    /**
     * @brief   Set the value of a configuration variable (generic type)
     * @param   section   The name of the section where the configuration
     *                    variable is located
     * @param   tag       The name of the configuration variable to set.
     * @param   value     The value to set
     *
     * This function is used to set the value of a configuration variable.
     * The type of the value may be any type that support streaming to string.
     * If the given configuration section or variable does not exist, it
     * is created.
     * Note that this function will not write anything back to the
     * associated configuration file. It will only set the value in memory.
     *
     * The valueUpdated signal will be emitted so that subscribers can get
     * notified when the value of a configuration variable is changed.
     */
    template <typename Rsp>
    void setValue(const std::string& section, const std::string& tag,
                  const Rsp& value)
    {
      std::ostringstream ss;
      ss << value;
      setValue(section, tag, ss.str());
    }

    /**
     * @brief   Set the value of a configuration variable (sequence container)
     * @param   section   The name of the section where the configuration
     *                    variable is located
     * @param   tag       The name of the configuration variable to set.
     * @param   c         The sequence to set
     *
     * This function is used to set the value of a configuration variable that
     * holds a sequence container (e.g. std::vector, std::list etc).
     * The type of the elements of the container may be any type that support
     * streaming to string.
     * If the given configuration section or variable does not exist, it
     * is created.
     * Note that this function will not write anything back to the
     * associated configuration file. It will only set the value in memory.
     *
     * The valueUpdated signal will be emitted so that subscribers can get
     * notified when the value of a configuration variable is changed.
     */
    template <template <typename, typename> class Container,
              typename Rsp>
    void setValue(const std::string& section, const std::string& tag,
                  const Container<Rsp, std::allocator<Rsp>>& c)
    {
      std::ostringstream ss;
      bool first_val = true;
      for (const auto& val : c)
      {
        if (!first_val)
        {
          ss << ",";
        }
        first_val = false;
        ss << val;
      }
      setValue(section, tag, ss.str());
    } /* setValue */

    /**
     * @brief   A signal that is emitted when a config value is updated
     * @param   section The config section of the update
     * @param   tag     The tag (variable name) of the update
     *
     * This signal is emitted whenever a configuration variable is changed
     * by calling the setValue function. It will only be emitted if the value
     * actually changes.
     */
    sigc::signal<void, const std::string&, const std::string&> valueUpdated;

  private:
    using Subscriber = std::function<void(const std::string&)>;
    struct Value
    {
      std::string             val;
      std::vector<Subscriber> subs;
    };
    typedef std::map<std::string, Value>  Values;
    typedef std::map<std::string, Values> Sections;

      // Really wanted to use classic_table() but it returns nullptr on Alpine
    static const std::ctype<char>::mask* empty_table()
    {
      static const auto table_size = std::ctype<char>::table_size;
      static std::ctype<char>::mask v[table_size];
      std::fill(&v[0], &v[table_size], 0);
      return &v[0];
    }

    struct empty_ctype : std::ctype<char>
    {
      static const mask* make_table(void) { return empty_table(); }
      empty_ctype(std::size_t refs=0) : ctype(make_table(), false, refs) {}
    };

    struct csv_whitespace : std::ctype<char>
    {
      static const mask* make_table()
      {
        auto tbl = empty_table();
        static std::vector<mask> v(tbl, tbl + table_size);
        v[' '] |= space;
        v[','] |= space;
        return &v[0];
      }
      csv_whitespace(std::size_t refs=0) : ctype(make_table(), false, refs) {}
    };

    Sections  sections;

    bool parseCfgFile(FILE *file);
    char *trimSpaces(char *line);
    char *parseSection(char *line);
    char *parseDelimitedString(char *str, char begin_tok, char end_tok);
    bool parseValueLine(char *line, std::string& tag, std::string& value);
    char *parseValue(char *value);
    char *translateEscapedChars(char *val);

    template <class T>
    bool setValueFromString(T& val, const std::string &str) const
    {
      std::istringstream ss(str);
      ss >> std::noskipws >> val;
      if(!ss.eof())
      {
        ss >> std::ws;
      }
      return !ss.fail() && ss.eof();
    }

    template <typename T>
    Value& getValueP(const std::string& section, const std::string& tag,
                     const T& def)
    {
      Values::iterator val_it = sections[section].find(tag);
      if (val_it == sections[section].end())
      {
        setValue(section, tag, def);
      }

      return sections[section][tag];
    } /* getValueP */

}; /* class Config */


} /* namespace */

#endif /* ASYNC_CONFIG_INCLUDED */



/*
 * This file has not been truncated
 */

