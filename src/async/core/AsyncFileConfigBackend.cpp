/**
@file	 AsyncFileConfigBackend.cpp
@brief   File-based configuration backend implementation
@author  Tobias Blomberg / SM0SVX, Rui Barreiros / CR7BPM
@date	 2025-09-19

This file contains the implementation of the file-based configuration backend
that reads INI-formatted configuration files.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2026 Tobias Blomberg / SM0SVX

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

#include "AsyncFileConfigBackend.h"

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

// Factory registration for file backend
static ConfigBackendSpecificFactory<FileConfigBackend> file_factory;

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

FileConfigBackend::FileConfigBackend(void)
  : ConfigBackend(false, 0), m_is_open(false)
{
} /* FileConfigBackend::FileConfigBackend */

FileConfigBackend::~FileConfigBackend(void)
{
  close();
} /* FileConfigBackend::~FileConfigBackend */

bool FileConfigBackend::open(const string& source)
{
  close();
  
  errno = 0;
  m_filename = source;

  FILE *file = fopen(source.c_str(), "r");
  if (file == NULL)
  {
    return false;
  }

  bool success = parseCfgFile(file);

  fclose(file);
  file = NULL;

  if (success)
  {
    m_is_open = true;
  }

  return success;
} /* FileConfigBackend::open */

void FileConfigBackend::close(void)
{
  m_sections.clear();
  m_filename.clear();
  m_is_open = false;
} /* FileConfigBackend::close */

bool FileConfigBackend::isOpen(void) const
{
  return m_is_open;
} /* FileConfigBackend::isOpen */

bool FileConfigBackend::getValue(const std::string& section, const std::string& tag,
                                 std::string& value) const
{
  if (!m_is_open)
  {
    return false;
  }

  Sections::const_iterator sec_it = m_sections.find(section);
  if (sec_it == m_sections.end())
  {
    return false;
  }

  Values::const_iterator val_it = sec_it->second.find(tag);
  if (val_it == sec_it->second.end())
  {
    return false;
  }

  value = val_it->second.val;
  return true;
} /* FileConfigBackend::getValue */

bool FileConfigBackend::setValue(const std::string& section, const std::string& tag,
                                 const std::string& value)
{
  if (!m_is_open)
  {
    return false;
  }

  Values &values = m_sections[section];
  values[tag].val = value;
  notifyValueChanged(section, tag, value);
  return true;
} /* FileConfigBackend::setValue */

list<string> FileConfigBackend::listSections(void) const
{
  list<string> section_list;
  
  if (!m_is_open)
  {
    return section_list;
  }

  for (Sections::const_iterator it = m_sections.begin(); it != m_sections.end(); ++it)
  {
    section_list.push_back(it->first);
  }
  return section_list;
} /* FileConfigBackend::listSections */

list<string> FileConfigBackend::listSection(const string& section) const
{
  list<string> tags;
  
  if (!m_is_open)
  {
    return tags;
  }
  
  Sections::const_iterator sec_it = m_sections.find(section);
  if (sec_it == m_sections.end())
  {
    return tags;
  }
  
  const Values& values = sec_it->second;
  for (Values::const_iterator it = values.begin(); it != values.end(); ++it)
  {
    tags.push_back(it->first);
  }
  
  return tags;
} /* FileConfigBackend::listSection */

std::string FileConfigBackend::getBackendType(void) const
{
  return "file";
} /* FileConfigBackend::getBackendType */

std::string FileConfigBackend::getBackendInfo(void) const
{
  return m_filename;
} /* FileConfigBackend::getBackendInfo */

/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

bool FileConfigBackend::parseCfgFile(FILE *file)
{
  char line[16384];
  int line_no = 0;
  string current_sec;
  string current_tag;
  
  while (fgets(line, sizeof(line), file) != 0)
  {
    ++line_no;
    char *l = trimSpaces(line);
    
    switch (l[0])
    {
      case 0:     // Ignore empty rows and
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
        current_sec = sec;
        current_tag = "";
        if (m_sections.count(current_sec) == 0)
        {
          m_sections[current_sec];  // Create a new empty section
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
        
        if (current_tag.empty())
        {
          cerr << "*** ERROR: Configuration file parse error. Line "
                  "continuation without previous value on line "
               << line_no << endl;
          return false;
        }
        assert(!current_sec.empty());
        
        Values &values = m_sections[current_sec];
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
        
        if (current_sec.empty())
        {
          cerr << "*** ERROR: Configuration file parse error. Value without "
                  "section on line " << line_no << endl;
          return false;
        }
        Values &values = m_sections[current_sec];
        current_tag = tag;
        values[current_tag].val = value;
        break;
      }
    }
  }
  
  return true;
} /* FileConfigBackend::parseCfgFile */

char *FileConfigBackend::trimSpaces(char *line)
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
} /* FileConfigBackend::trimSpaces */

char *FileConfigBackend::parseSection(char *line)
{
  return parseDelimitedString(line, '[', ']');
} /* FileConfigBackend::parseSection */

char *FileConfigBackend::parseDelimitedString(char *str, char begin_tok, char end_tok)
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
  
  return str + 1;
} /* FileConfigBackend::parseDelimitedString */

bool FileConfigBackend::parseValueLine(char *line, string& tag, string& value)
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
} /* FileConfigBackend::parseValueLine */

char *FileConfigBackend::parseValue(char *value)
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
} /* FileConfigBackend::parseValue */

char *FileConfigBackend::translateEscapedChars(char *val)
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
} /* FileConfigBackend::translateEscapedChars */

/*
 * This file has not been truncated
 */
