/**
@file	 EchoLinkStationData.h
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


#ifndef ECHOLINK_STATION_DATA_INCLUDED
#define ECHOLINK_STATION_DATA_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <netinet/in.h>

#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncIpAddress.h>


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

namespace EchoLink
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
@brief	A class for representing data for a station
@author Tobias Blomberg
@date   2003-04-13

This class is used to represent data about a station.
*/
class StationData
{
  public:
    /**
     * The status of the station in the directory server
     */
    typedef enum
    {
      STAT_UNKNOWN, ///< The state is unknown
      STAT_OFFLINE, ///< The state is offline
      STAT_ONLINE,  ///< The state is online
      STAT_BUSY     ///< The state is busy
    } Status;
    
    static const int MAXCALL  = 15; ///< The maximum length of a callsign
    static const int MAXDATA  = 45; ///< The maximum length of the data field
    static const int MAXDESC  = 27; ///< The maximum length of a description
    static const int MAXID    = 7;  ///< The maximum length of the id field
    static const int MAXIP    = 20; ///< The maximum length of the ip address
    
    /**
     * @brief 	Translate a status code to a string
     * @param 	status The status code to translate
     * @return  Returns the string representation of the given status code
     */
    static std::string statusStr(Status status);
    
    /**
     * @brief Default constructor
     */
    StationData(void);
    
    /**
     * @brief Copy constructor
     */
    StationData(const StationData& rhs) { *this = rhs; }
    
    /**
     * @brief Clear the contents and reset to default values
     */
    void clear(void);

    /**
     * @brief 	Set the callsign
     * @param 	callsign The callsign to set
     */
    void setCallsign(const std::string& callsign);
    
    /**
     * @brief 	Get the callsign
     * @return	Returns the callsign
     */
    const std::string& callsign(void) const { return m_callsign; }
    
    /**
     * @brief Set station data from a string as represented in the directory
     *	      server
     * @param data The data to set
     *
     * This function is used to set status, time and description from a string
     * that have a representation like in the reply from the directory server.
     * The only use for this function is probably when parsing a reply from the
     * directory server. Use setStatus, setTime and setDescription for normal
     * use.
     */
    void setData(const char *data);
    
    /**
     * @brief 	Set the status
     * @param 	status The new status to set
     */
    void setStatus(Status status) { m_status = status; }
    
    /**
     * @brief 	Get the status
     * @return	Returns the status
     */
    Status status(void) const { return m_status; }
    
    /**
     * @brief 	Return the string representation of the status
     * @return	Returns a string representation of the status
     */
    std::string statusStr(void) const { return statusStr(m_status); }
    
    /**
     * @brief 	Set the time
     * @param 	time The time to set
     */
    void setTime(const std::string& time) { m_time = time; }
    
    /**
     * @brief 	Get the time
     * @return	Returns the time
     */
    const std::string& time(void) const { return m_time; }
    
    /**
     * @brief Set the description/location string
     * @param desc  The description string to set
     */
    void setDescription(const std::string& desc) { m_description = desc; }
    
    /**
     * @brief 	Get the description/location string
     * @return	Returns the description/location string
     */
    const std::string& description(void) const { return m_description; }
    
    /**
     * @brief Set the EchoLink ID number
     * @param id The new id
     */
    void setId(int id) { m_id = id; }
    
    /**
     * @brief 	Get the EchoLink ID number
     * @return	Returns the EchoLink ID number
     */
    int id(void) const { return m_id; }
    
    /**
     * @brief 	Set the IP address
     * @param 	ip The IP address to set
     */
    void setIp(const Async::IpAddress& ip) { m_ip = ip; }
    
    /**
     * @brief 	Get the IP address
     * @return	Returns the IP address
     */
    const Async::IpAddress ip(void) const { return m_ip; }
    
    /**
     * @brief 	Get the string representation of the IP address
     * @return	Returns the string representation of the IP address
     */
    std::string ipStr(void) const { return m_ip.toString(); }

    /**
     *
     * @brief	Get the code representation of the callsign
     * @return  Returns the code representation of the callsign
     *
     * The code representation is the callsign mapped to digits only.
     * The mapping is done using the "phone method".
     * ABC=2, DEF=3, GHI=4, JKL=5, MNO=6, PQRS=7, TUV=8, WXYZ=9.
     * Digits are mapped to its corresponding digit.
     * Star is ignored.
     * All other characters are mapped to digit 1.
     */
    std::string code(void) const { return m_code; }
    
    /**
     * @brief 	Assignment operator
     * @param 	rhs Right Hand Side expression
     * @return	Returns a reference to this object
     */
    StationData& operator=(const StationData& rhs);
    
    bool operator<(const StationData &rhs) const
    {
      return m_callsign < rhs.m_callsign;
    }

    /**
     * @brief Output stream operator
     * @param os The stream to output data to
     * @param station The station data to output to the stream
     */
    friend std::ostream& operator<<(std::ostream& os,
	const StationData& station);
    
    
  protected:
    
  private:
    std::string       m_callsign;
    Status    	      m_status;
    std::string       m_time;
    std::string       m_description;
    int       	      m_id;
    Async::IpAddress  m_ip;
    std::string       m_code;
  
    void removeTrailingSpaces(std::string& str);
    std::string callToCode(const std::string& call);

};  /* class StationData */


} /* namespace */

#endif /* ECHOLINK_STATION_DATA_INCLUDED */



/*
 * This file has not been truncated
 */

