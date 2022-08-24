/**
@file	 EchoLinkStationData.cpp
@brief   Contains a class that represent station data
@author  Tobias Blomberg
@date	 2003-04-13

This file contains a class that is used to represent the data for a station.

\verbatim
EchoLib - A library for EchoLink communication
Copyright (C) 2003  Tobias Blomberg

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

#include <cstring>
#include <cstdlib>

#include <sys/types.h>


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

#include "EchoLinkStationData.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace EchoLink;



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

string StationData::statusStr(Status status)
{
  const char *str;
  
  switch (status)
  {
    case STAT_ONLINE:
      str = "ON";
      break;
      
    case STAT_BUSY:
      str = "BUSY";
      break;
      
    case STAT_OFFLINE:
      str = "OFF";
      break;
      
    default:
      str="?";
      break;
  }
  
  return str;
  
} /* StationData::statusStr */


StationData::StationData(void)
{
  clear();
} /* StationData::StationData */


/*
 *------------------------------------------------------------------------
 * Method:    StationData::StationData
 * Purpose:   Constructor for the StationData struct
 * Input:     
 * Output:    None
 * Author:    Tobias Blomberg
 * Created:   2003-03-08
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
void StationData::clear(void)
{
  m_callsign = "";
  m_status = STAT_UNKNOWN;
  m_time = "";
  m_description = "";
  m_id = -1;
  m_code = "";
} /* StationData::clear */


void StationData::setCallsign(const string& callsign)
{
  m_callsign = callsign;
  m_code = callToCode(callsign);
} /* StationData::setCallsign  */


void StationData::setData(const char *data)
{
  char str[MAXDATA];
  
  const char *end_desc = strrchr(data, '[');
  if (end_desc != 0)
  {
    if (strstr(end_desc+1, "ON"))
    {
      m_status = STAT_ONLINE;
    }
    else if (strstr(end_desc+1, "BUSY"))
    {
      m_status = STAT_BUSY;
    }
    else
    {
      m_status = STAT_UNKNOWN;
    }
    
    const char *space = strchr(end_desc, ' ');
    if (space != 0)
    {
      strncpy(str, space+1, 5);
      str[5] = 0;
      m_time = str;
    }
  }
  else
  {
    end_desc = data + strlen(data);
  }
  
  strncpy(str, data, end_desc-data);
  str[end_desc-data] = 0;
  m_description = str;
  removeTrailingSpaces(m_description);
  
} /* StationData::setData */


StationData& StationData::operator=(const StationData& rhs)
{
  m_callsign = rhs.m_callsign;
  m_status = rhs.m_status;
  m_time = rhs.m_time;
  m_description = rhs.m_description;
  m_id = rhs.m_id;
  m_ip = rhs.m_ip;
  m_code = rhs.m_code;
  
  return *this;
  
} /* StationData::operator= */




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
void StationData::removeTrailingSpaces(string& str)
{
  int pos = str.find_last_not_of(' ');
  if (pos >= 0)
  {
    ++pos;
    if (pos < static_cast<int>(str.size()))
    {
      str.erase(pos);
    }
  }
  else
  {
    str = "";
  }
} /* StationData::removeTrailingSpaces */


string StationData::callToCode(const string& call)
{
  string code;

  for (unsigned i=0; i<call.length(); ++i)
  {
    char digit;
    if ((call[i] >= 'A') && (call[i] <= 'R'))
    {
      digit = (call[i] - 'A') / 3 + 2 + '0';
    }
    else if ((call[i] >= 'S') && (call[i] <= 'Z'))
    {
      digit = min((call[i] - 'A' - 1) / 3 + 2 + '0', int('9'));
    }
    else if (isdigit(call[i]))
    {
      digit = call[i];
    }
    else if (call[i] == '*')
    {
      continue;
    }
    else
    {
      digit = '1';
    }
    code += digit;
  }

  return code;
  
} /* StationData::callToCode  */





/*
 * This file has not been truncated
 */

