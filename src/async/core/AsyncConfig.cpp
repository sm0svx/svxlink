/**
@file	 AsyncConfig.cpp
@brief   A class for reading "INI-foramtted" configuration files
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-17

This file contains a class that is used to read configuration files that is
in the famous MS Windows INI file format. An example of a configuration file
is shown below.

\include test.cfg

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2025 Tobias Blomberg / SM0SVX

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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include <iostream>
#include <cassert>
#include <cstring>


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

#include "AsyncConfig.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


Config::~Config(void)
{
} /* Config::~Config */


bool Config::open(const string& name)
{
  errno = 0;

  FILE *file = fopen(name.c_str(), "r");
  if (file == NULL)
  {
    return false;
  }

  bool success = parseCfgFile(file);

  fclose(file);
  file = NULL;

  return success;

} /* Config::open */


bool Config::getValue(const std::string& section, const std::string& tag,
                      std::string& value, bool missing_ok) const
{
  Sections::const_iterator sec_it = sections.find(section);
  if (sec_it == sections.end())
  {
    return missing_ok;
  }

  Values::const_iterator val_it = sec_it->second.find(tag);
  if (val_it == sec_it->second.end())
  {
    return missing_ok;
  }

  value = val_it->second.val;
  return true;
} /* Config::getValue */


bool Config::getValue(const std::string& section, const std::string& tag,
                      char& value, bool missing_ok) const
{
  Sections::const_iterator sec_it = sections.find(section);
  if (sec_it == sections.end())
  {
    return missing_ok;
  }

  Values::const_iterator val_it = sec_it->second.find(tag);
  if (val_it == sec_it->second.end())
  {
    return missing_ok;
  }

  if (val_it->second.val.size() != 1)
  {
    return false;
  }

  value = val_it->second.val[0];
  return true;
} /* Config::getValue */


const string &Config::getValue(const string& section, const string& tag) const
{
  static const string empty_strng;
  
  Sections::const_iterator sec_it = sections.find(section);
  if (sec_it == sections.end())
  {
    return empty_strng;
  }

  Values::const_iterator val_it = sec_it->second.find(tag);
  if (val_it == sec_it->second.end())
  {
    return empty_strng;
  }

  return val_it->second.val;
} /* Config::getValue */


list<string> Config::listSections(void)
{
  list<string> section_list;
  for (Sections::const_iterator it=sections.begin(); it!=sections.end(); ++it)
  {
    section_list.push_back((*it).first);
  }
  return section_list;
} /* Config::listSections */


list<string> Config::listSection(const string& section)
{
  list<string> tags;
  
  if (sections.count(section) == 0)
  {
    return tags;
  }
  
  Values& values = sections[section];
  Values::iterator it = values.begin();
  for (it=values.begin(); it!=values.end(); ++it)
  {
    tags.push_back(it->first);
  }
  
  return tags;
  
} /* Config::listSection */


void Config::setValue(const std::string& section, const std::string& tag,
                      const std::string& value)
{
  Values &values = sections[section];
  if (value != values[tag].val)
  {
    values[tag].val = value;
    valueUpdated(section, tag);
    for (const auto& func : values[tag].subs)
    {
      func(value);
    }
  }
} /* Config::setValue */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
bool Config::parseCfgFile(FILE *file)
{
  char line[16384];
  int line_no = 0;
  string current_sec;
  string current_tag;
  
  while (fgets(line, sizeof(line), file) != 0)
  {
    ++line_no;
    char *l = trimSpaces(line);
    //printf("%s\n", l);
    switch (l[0])
    {
      case 0: 	  // Ignore empty rows and
      case '#':   // rows starting with a #-character == comments
      	break;
      
      case '[':
      {
      	char *sec = parseSection(l);
	if ((sec == 0) || (sec[0] == 0))
	{
	  cerr << "*** ERROR: Configuration file parse error. Illegal section "
	      	  "name syntax on line " << line_no << endl;
	  return false;
	}
	//printf("New section=%s\n", sec);
	current_sec = sec;
	current_tag = "";
	if (sections.count(current_sec) == 0)
	{
	  sections[current_sec];	// Create a new empty section
	  //cerr << "*** ERROR: Configuration file parse error: Section "
	  //    	  "previously defined on line " << line_no << endl;
	  //return false;
	}
      	break;
      }
      
      case '"':
      {
      	char *val = parseValue(l);
	if (val == 0)
	{
	  cerr << "*** ERROR: Configuration file parse error. Illegal value "
	      	  "syntax on line " << line_no << endl;
	  return false;
	}
	//printf("Continued line=\"%s\"", val);
	
	if (current_tag.empty())
	{
	  cerr << "*** ERROR: Configuration file parse error. Line "
	      	  "continuation without previous value on line "
	       << line_no << endl;
	  return false;
	}
	assert(!current_sec.empty());
	
	Values &values = sections[current_sec];
	string& value = values[current_tag].val;
	value += val;
	break;
      }
      
      default:
      {
      	string tag, value;
      	if (!parseValueLine(l, tag, value))
	{
	  cerr << "*** ERROR: Configuration file parse error. Illegal value "
	      	  "line syntax on line " << line_no << endl;
	  return false;
	}
	//printf("tag=\"%s\"  value=\"%s\"\n", tag.c_str(), value.c_str());
	
	if (current_sec.empty())
	{
	  cerr << "*** ERROR: Configuration file parse error. Value without "
	      	  "section on line " << line_no << endl;
	  return false;
	}
	Values &values = sections[current_sec];
	current_tag = tag;
	values[current_tag].val = value;
      	break;
      }
    }
  }
  
  return true;
  
} /* Config::parseCfgFile */


char *Config::trimSpaces(char *line)
{
  char *begin = line;
  while ((*begin != 0) && isspace(*begin))
  {
    ++begin;
  }
  
  char *end = begin + strlen(begin);
  while ((end != begin) && (isspace(*end) || (*end == 0)))
  {
    *end-- = 0;
  }
  
  return begin;
  
} /* Config::trimSpaces */


char *Config::parseSection(char *line)
{
  return parseDelimitedString(line, '[', ']');
} /* Config::parseSection */


char *Config::parseDelimitedString(char *str, char begin_tok, char end_tok)
{
  if (str[0] != begin_tok)
  {
    return 0;
  }
  
  char *end = str + strlen(str) - 1;
  if (*end != end_tok)
  {
    return 0;
  }
  *end = 0;
  
  /*
  if (end == str+1)
  {
    return 0;
  }
  */
  
  return str + 1;
  
} /* Config::parseDelimitedString */


bool Config::parseValueLine(char *line, string& tag, string& value)
{
  char *eq = strchr(line, '=');
  if (eq == 0)
  {
    return false;
  }
  *eq = 0;
  
  tag = trimSpaces(line);
  char *val = parseValue(eq + 1);
  if (val == 0)
  {
    return false;
  }
  
  value = val;
  
  return true;
  
} /* Config::parseValueLine */


char *Config::parseValue(char *value)
{
  value = trimSpaces(value);
  if (value[0] == '"')
  {
    value = parseDelimitedString(value, '"', '"');
  }
  
  if (value == 0)
  {
    return 0;
  }
  
  return translateEscapedChars(value);
  
} /* Config::parseValue */


char *Config::translateEscapedChars(char *val)
{
  char *head = val;
  char *tail = head;
  
  while (*head != 0)
  {
    if (*head == '\\')
    {
      ++head;
      switch (*head)
      {
      	case 'n':
	  *tail = '\n';
	  break;
	  
      	case 'r':
	  *tail = '\r';
	  break;
	  
      	case 't':
	  *tail = '\t';
	  break;
	  
      	case '\\':
	  *tail = '\\';
	  break;
	  
      	case '"':
	  *tail = '"';
	  break;
	  
      	default:
	  return 0;
      }
    }
    else
    {
      *tail = *head;
    }
    ++head;
    ++tail;
  }
  *tail = 0;
  
  return val;
  
} /* Config::translateEscapedChars */


/*
 * This file has not been truncated
 */
