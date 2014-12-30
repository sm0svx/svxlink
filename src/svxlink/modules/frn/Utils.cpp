/**
@file	 Utils.cpp
@brief   Helpers for qso frn module
@author  sh123
@date	 2014-12-30

This file contains a class that implementes the things needed for one
EchoLink Qso.

\verbatim
A module (plugin) for the multi purpose tranciever frontend system.
Copyright (C) 2004-2014 Tobias Blomberg / SM0SVX

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
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>


namespace FrnUtils
{

std::istream& safeGetline(std::istream& is, std::string& t)
{
  t.clear();
  std::istream::sentry se(is, true);
  std::streambuf* sb = is.rdbuf();

  for(;;) {
    int c = sb->sbumpc();
    switch (c) {
      case '\n':
        if(sb->sgetc() == '\r')
          sb->sbumpc();
        return is;
      case '\r':
        if(sb->sgetc() == '\n')
        {
          sb->sbumpc();
          return is;
        }
        t += (char)c;
        break;
      case EOF:
        if(t.empty())
          is.setstate(std::ios::eofbit);
        return is;
      default:
       t += (char)c;
       break;
    }
  }
}


bool hasLine(std::istringstream& is)
{
  return (is.str().find('\n') != std::string::npos);
}


bool hasWinNewline(std::istringstream& is)
{
  return !(is.str().find("\r\n") == std::string::npos && 
           is.str().find("\n\r") == std::string::npos);
}


} // namspace FrnUtils

/*
 * This file has not been truncated
 */

