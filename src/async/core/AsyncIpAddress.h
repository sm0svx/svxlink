/**
@file	 AsyncIpAddress.h
@brief   Platform independent representation of an IP address
@author  Tobias Blomberg
@date	 2003-04-12

Contains a class for representing an IP address in an OS independent way.
Rigt now it can only handle IPv4 addresses.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2022 Tobias Blomberg

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

#ifndef ASYNC_IP_ADDRESS_INCLUDED
#define ASYNC_IP_ADDRESS_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <netinet/in.h>

#include <string>
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
 * @brief A class for representing an IP address in an OS independent way.
 */
class IpAddress
{
  public:
    /**
     * @brief The type for the OS specific representation of an IP address
     */
    typedef struct in_addr Ip4Addr;
    
    /**
     * @brief Default constructor for the IpAddress class.
     */
    IpAddress(void);
    
    /**
     * @brief Constructor for the IpAddress class.
     * @param addr The string representation of an IP address
     */
    IpAddress(const std::string& addr);
     
    /**
     * @brief Constructor for the IpAddress class.
     * @param addr The IP address in OS specific representation
     */
    IpAddress(const Ip4Addr& addr);
    
    /**
     * @brief Copy contructor
     * @param addr An IpAddress object to construct the new object from
     */
    IpAddress(const IpAddress& addr) { *this = addr; }
    
    /**
     * @brief Destructor
     */
    ~IpAddress(void) {}
    
    /**
     * @brief Return the IP address in OS specific representation
     * @return The IP address
     */
    Ip4Addr ip4Addr(void) const { return m_addr; }
    
    /**
     * @brief 	Check if this is a unicast IP address
     * @return	Return \em true if this is a unicast address or \em false
     *	      	if it is some other type.
     */
    bool isUnicast(void) const;
    
    /**
     * @brief 	Check if the IP address is within the given netmask
     * @param 	subnet	The subnet to use in the check. The subnet should
     *	      	      	be given on the form a.b.c.d/m (e.g. 192.168.1.0/24).
     * @return	Return \em true if within the given subnet or \em false
     *	      	if it is not.
     */
    bool isWithinSubet(const std::string& subnet) const;

    /**
     * @brief	Check if an invalid IP address has been assigned
     * @return	Return \em true if this is an invalid address or \em false
     *		if a valid address has been assigned.
     */
    bool isEmpty(void) const { return (m_addr.s_addr == INADDR_NONE); }

    /**
     * @brief	Invalidate the IP address value
     */    
    void clear(void) { m_addr.s_addr = INADDR_NONE; }
    
    /**
     * @brief 	Return the string representation of the IP address.
     * @return  The IP address string
     */
    std::string toString(void) const;

    /**
     * @brief   Set the IP address
     * @param   addr The IP address to set
     */
    void setIp(const Ip4Addr& addr) { m_addr = addr; }

    /**
     * @brief   Set the IP address from a string
     * @param   str The string to parse (e.g. "192.168.0.1")
     * @return  Returns \em true on success or else \em false
     */
    bool setIpFromString(const std::string &str);

    /**
     * @brief 	Assignment operator.
     * @param 	rhs The address object to assign to this object
     * @return  Returns the new IP address
     */
    IpAddress& operator=(const IpAddress& rhs)
    {
      m_addr = rhs.m_addr;
      return *this;
    }
    
    /**
     * @brief 	Equality operator
     * @param 	rhs Right hand side expression
     * @return  Returns \em true if the right hand side object is
     *          equal to this object, or else returns \em false.
     */
    bool operator==(const IpAddress& rhs) const
    {
      return m_addr.s_addr == rhs.m_addr.s_addr;
    }
    
    /**
     * @brief 	Unequality operator
     * @param 	rhs Right hand side expression
     * @return  Returns \em true if the right hand side object is
     *          unequal to this object, or else returns \em false.
     */
    bool operator!=(const IpAddress& rhs) const
    {
      return !(*this == rhs);
    }
    
    /**
     * @brief 	Less than operator
     * @param 	rhs Right hand side expression
     * @return  Returns \em true if the right hand side object is
     *          less than this object, or else returns \em false.
     */
    bool operator<(const IpAddress& rhs) const
    {
      return m_addr.s_addr < rhs.m_addr.s_addr;
    }
    
    /**
     * @brief Output stream operator
     * @param os The stream to output data to
     * @param ip The IP address to output to the stream
     */
    friend std::ostream& operator<<(std::ostream& os,
	const Async::IpAddress& ip);
    
    /**
     * @brief Input stream operator
     * @param is The stream to input data from
     * @param ip The IP address object to store information in
     */
    friend std::istream& operator>>(std::istream& is,
	Async::IpAddress& ip);
    
  protected:
    
  private:
    Ip4Addr m_addr;
  
};  /* class IpAddress */


std::ostream& operator<<(std::ostream& os, const IpAddress& ip);
std::istream& operator>>(std::istream& is, IpAddress& ip);


} /* namespace */


#endif /* ASYNC_IP_ADDRESS_INCLUDED */



/*
 * This file has not been truncated
 */

