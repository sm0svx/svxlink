/**
@file	 AsyncIpAddress.cpp
@brief   Platform independent representation of an IP address
@author  Tobias Blomberg
@date	 2003-04-12

Contains a class for representing an IP address in an OS independent way.
Rigt now it can only handle IPv4 addresses.

\verbatim
Async - A library for programming event driven applications
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cmath>
#include <algorithm>


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

#include "AsyncIpAddress.h"



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
IpAddress::IpAddress(void)
{
  m_addr.s_addr = INADDR_NONE;
} /* IpAddress::IpAddress */


IpAddress::IpAddress(const string& addr)
{
  setIpFromString(addr);
} /* IpAddress::IpAddress */


IpAddress::IpAddress(const Ip4Addr& addr)
  : m_addr(addr)
{
  
} /* IpAddress::IpAddress */


bool IpAddress::isUnicast(void) const
{
  bool is_unicast;
  is_unicast  = (ntohl(m_addr.s_addr) & 0x80000000) == 0x00000000;
  is_unicast |= (ntohl(m_addr.s_addr) & 0xc0000000) == 0x80000000;
  is_unicast |= (ntohl(m_addr.s_addr) & 0xe0000000) == 0xc0000000;
  
  return is_unicast;
  
} /* IpAddress::isUnicast */


bool IpAddress::isWithinSubet(const std::string& subnet) const
{
  string::const_iterator slash;
  slash = find(subnet.begin(), subnet.end(), '/');
  if (slash == subnet.end())
  {
    return false;
  }
  
  string ip_str(subnet.begin(), slash);
  Ip4Addr ip;
  if (inet_aton(ip_str.c_str(), &ip) == 0)  // Address is invalid
  {
    return false;
  }
  
  if (++slash == subnet.end())
  {
    return false;
  }
  string mask_str(slash, subnet.end());
  uint32_t mask = 0xffffffff ^ ((uint32_t)pow(2.0, 32-atoi(mask_str.c_str())) - 1);

  return (ntohl(m_addr.s_addr) & mask) == (ntohl(ip.s_addr) & mask);
 
} /* IpAddress::isWithinSubet */


string IpAddress::toString(void) const
{
  return inet_ntoa(m_addr);
} /* IpAddress::toString */


bool IpAddress::setIpFromString(const string &str)
{
  if (inet_aton(str.c_str(), &m_addr) == 0)  // Address is invalid
  {
    m_addr.s_addr = INADDR_NONE;
    return false;
  }
  return true;
} /* IpAddress::setIpFromString */


std::ostream& Async::operator<<(std::ostream& os, const Async::IpAddress& ip)
{
  return os << ip.toString();
} /* Async::operator<< */



std::istream& Async::operator>>(std::istream& is, Async::IpAddress& ip)
{
  string str;
  is >> str;
  ip.setIpFromString(str);
  return is;
} /* Async::operator<< */



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







/*
 * This file has not been truncated
 */

