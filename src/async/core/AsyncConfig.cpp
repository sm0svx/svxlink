/**
@file	 AsyncConfig.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-17

A_detailed_description_for_this_file

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003 Tobias Blomberg / SM0SVX

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
#include <ctype.h>

#include <iostream>


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
Config::Config(void)
  : file(0)
{

} /* Config::Config */


Config::~Config(void)
{
  fclose(file);
} /* Config::~Config */


bool Config::open(const string& name)
{
  if (access(name.c_str(), R_OK | W_OK) != 0)
  {
    perror("Config::open");
    return false;
  }
  
  file = fopen(name.c_str(), "r+");
  if (file == 0)
  {
    perror("fopen");
    return false;
  }
  
  if (!parseCfgFile())
  {
    fclose(file);
    file = 0;
    return false;
  }
  
  return true;
  
} /* Config::open */


bool Config::getValue(const string& section, const string& tag, string& value)
{
  if (sections.count(section) == 0)
  {
    return false;
  }
  
  Values& values = sections[section];
  if (values.count(tag) == 0)
  {
    return false;
  }
  
  value = values[tag];
  
  return true;
} /* Config::getValue */





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
bool Config::parseCfgFile(void)
{
  char line[16384];
  int line_no = 0;
  string current_sec;
  string current_tag;
  
  while (fgets(line, sizeof(line), file) != 0)
  {
    ++line_no;
    char *l = trimSpaces(line);
    printf("%s\n", l);
    switch (l[0])
    {
      case 0:
      	break;
      
      case '[':
      {
      	char *sec = parseSection(l);
	if (sec == 0)
	{
	  cerr << "*** Config file parse error: Illegal section name syntax "
	      	  "on line " << line_no << endl;
	  return false;
	}
	printf("New section=%s\n", sec);
	current_sec = sec;
	current_tag = "";
	if (sections.count(current_sec) > 0)
	{
	  cerr << "*** Config file parse error: Section previously defined "
	      	  "on line " << line_no << endl;
	  return false;
	}
	sections[current_sec];	// Create a new empty section
      	break;
      }
      
      case '"':
      {
      	char *val = parseValue(l);
	if (val == 0)
	{
	  cerr << "*** Config file parse error: Illegal value syntax "
	      	  "on line " << line_no << endl;
	  return false;
	}
	printf("Continued line=\"%s\"", val);
	
	if (current_tag.empty())
	{
	  cerr << "*** Config file parse error: Line continuation without "
	      	  "previous value on line " << line_no << endl;
	  return false;
	}
	assert(!current_sec.empty());
	
	Values &values = sections[current_sec];
	string& value = values[current_tag];
	value += val;
	break;
      }
      
      default:
      {
      	string tag, value;
      	if (!parseValueLine(l, tag, value))
	{
	  cerr << "*** Config file parse error: Illegal value line syntax "
	      	  "on line " << line_no << endl;
	  return false;
	}
	printf("tag=\"%s\"  value=\"%s\"\n", tag.c_str(), value.c_str());
	
	if (current_sec.empty())
	{
	  cerr << "*** Config file parse error: Value without section "
	      	  "on line " << line_no << endl;
	  return false;
	}
	Values &values = sections[current_sec];
	current_tag = tag;
	values[current_tag] = value;
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
  
  if (end == str+1)
  {
    return 0;
  }
  
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

