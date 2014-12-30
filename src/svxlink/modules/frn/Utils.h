/**
@file	 Utils.h
@brief   Helpers for QSO FRN module
@author  sh123
@date	 2014-12-30

This file contains a class that implementes the things needed for one
Frn Qso.

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

namespace FrnUtils
{

/**
 * @brief Checks if there is a line in string stream
 *
 * This functions checks if string stream has a line, returns true
 * when line has been found.
 */
bool hasLine(std::istringstream& is);

/** 
 * @brief Checks if string stream has windows style newline
 *
 * This function returns true if windows style newline has been found
 * in given stream.
 */
bool hasWinNewline(std::istringstream& is);

/**
 * @brief Returns line from string stream halding also windows newlines
 *
 * This function is similar to std::getline, but takes care of windows
 * style new lines.
 */
std::istream& safeGetline(std::istream& is, std::string& t);

} // namespace FrnUtils

