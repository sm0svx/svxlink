/**
@file	 EchoLinkDirectory.h
@brief   Contains a class to access an EchoLink directory server
@author  Tobias Blomberg
@date	 2003-03-08

This file contains a class that is used to connect to an EchoLink directory
server. For usage instructions, see the class documentation for
EchoLink::Directory.

\verbatim
EchoLib - A library for EchoLink communication
Copyright (C) 2003  Tobias Blomberg / SM0SVX

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

/** @example EchoLinkDirectory_demo.cpp
An example of how to use the EchoLink::Directory class
*/


#ifndef ECHOLINK_DIRECTORY_INCLUDED
#define ECHOLINK_DIRECTORY_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

#include <string>
#include <list>
#include <vector>
#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpClient.h>
#include <AsyncTimer.h>
#include <EchoLinkStationData.h>


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

class Cmd;

namespace EchoLink
{
  class DirectoryCon;
};


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
@brief	A class for accessing an EchoLink directory server
@author Tobias Blomberg
@date   2003-03-08

Use this class to access an EchoLink directory server. The primary purpose of
the EchoLink directory server is to map between callsigns and IP-addresses. It
is also used to see which stations are online. An example usage that lists all
connected stations is shown below.

\include EchoLinkDirectory_demo.cpp
*/
class Directory : public sigc::trackable
{
  public:
    static const unsigned MAX_DESCRIPTION_SIZE = 27;
    
    /**
     * @brief 	Constructor
     * @param 	servers     The EchoLink directory servers to connect to
     * @param 	callsign    The callsign to register in the server
     * @param 	password    The password for the given callsign
     * @param 	description A description/location string
     * @param   bind_ip     The source IP address to use
     */
    Directory(const std::vector<std::string>& servers,
              const std::string& callsign,
      	      const std::string& password, const std::string& description="",
              const Async::IpAddress &bind_ip=Async::IpAddress());
  
    /**
     * @brief 	Destructor
     */
    ~Directory(void);
    
    /**
     * @brief 	Login to the directory server and set status to online
     *
     * Use this function to login to the directory server and set the status
     * to online. The registration will automatically be refreshed to keep
     * the registration from timing out.
     */
    void makeOnline(void);
    
    /**
     * @brief 	Login to the directory server and set status to busy
     *
     * Use this function to login to the directory server and set the status
     * to busy. The registration will automatically be refreshed to keep
     * the registration from timing out.
     */
    void makeBusy(void);
    
    /**
     * @brief 	Logout from the directory server
     */
    void makeOffline(void);
    
    /**
     * @brief 	Refresh the current registration in the directory server
     */
    void refreshRegistration(void) { onRefreshRegistration(0); }
    
    /**
     * @brief 	Return the current status of the registration
     * @return	Returns the status
     */
    StationData::Status status(void) const { return current_status; }
    
    /**
     * @brief 	Return the current status of the registration in string
     *	      	representation
     * @return	Returns a string representing the current status
     */
    std::string statusStr(void) const
    { 
      return StationData::statusStr(current_status);
    }
    
    /**
     * @brief Get the station list from the directory server
     *
     * Use this function to initiate a transfer of the station list from the
     * directory server. When the list has been completely transfered the
     * \em Directory::stationListUpdated signal will be emitted. If this
     * function is called while a previous \em getCalls is in progress, the
     * request will be ignored.
     *
     * After the transfer is done. There may be a server message to read. Get
     * this message by using the Directory::message function.
     */
    void getCalls(void);
    
    /**
     * @brief Set the hostname or IP-address of the EchoLink servers to use
     * @param servers The new servers to use
     */
    void setServers(const std::vector<std::string>& servers);
    
    /**
     * @brief 	Get the name of the remote host
     * @return	Returns the name of the remote host
     */
    const std::vector<std::string>& servers(void) const { return the_servers; }
    
    /**
     * @brief Set the callsign to use when logging in to the server
     * @param callsign The new callsign to use
     */
    void setCallsign(const std::string& callsign);
    
    /**
     * @brief 	Get the callsign that is used when logging in to the server
     * @return	Return the callsign
     */
    const std::string& callsign(void) const { return the_callsign; }
    
    /**
     * @brief 	Set the password to use when logging in to the server
     * @param 	password The new password to use
     */
    void setPassword(const std::string& password);

    /**
     * @brief 	Get the password that is used when logging in to the server
     * @return	Return the password
     */
    const std::string& password(void) const { return the_password; }
    
    /**
     * @brief 	Set the description to register in the server
     * @param 	description The new description to use
     *
     * Use this function to set the description used when registering in the
     * directory server. The description will not be updated in the directory
     * server until makeOnline, makeBusy or refreshRegistration is called.
     */
    void setDescription(const std::string& description);

    /**
     * @brief 	Get the description that is used when registering in the server
     * @return	Return the description
     */
    const std::string& description(void) const { return the_description; }
    
    /**
     * @brief 	Get a list of all active links
     * @return	Returns a reference to a list of StationData objects
     *
     * Use this function to get a list of all active links. Links are stations
     * where the callsign end with "-L". For this function to return anything,
     * a previous call to Directory::getCalls must have been made.
     */
    const std::list<StationData>& links(void) const { return the_links; }
    
    /**
     * @brief 	Get a list of all active repeasters
     * @return	Returns a reference to a list of StationData objects
     *
     * Use this function to get a list of all active repeaters. Repeaters are
     * stations where the callsign end with "-R". For this function to return
     * anything, a previous call to Directory::getCalls must have been made.
     */
    const std::list<StationData>& repeaters(void) const
    {
      return the_repeaters;
    }
    
    /**
     * @brief 	Get a list of all active conferences
     * @return	Returns a reference to a list of StationData objects
     *
     * Use this function to get a list of all active conferences. Conferences
     * are stations where the callsign is surrounded with "*". For this function
     * to return anything, a previous call to Directory::getCalls must have been
     * made.
     */
    const std::list<StationData>& conferences(void) const
    {
      return the_conferences;
    }
    
    /**
     * @brief 	Get a list of all active "normal" stations
     * @return	Returns a reference to a list of StationData objects
     */
    const std::list<StationData>& stations(void) const { return the_stations; }
    
    /**
     * @brief 	Get the message returned by the directory server
     * @return	Returns the message string (may be empty)
     *
     * This function is used to get the message returned by the directory server
     * after getting the station list. It is valid until a \em getCalls function
     * call is made again.
     */
    const std::string& message(void) const { return the_message; }
    
    /**
     * @brief 	Find a callsign in the station list
     * @param 	call  The callsign to find
     * @return	Returns a pointer to a StationData object if the callsign was
     *	      	found. Otherwise a NULL-pointer is returned.
     */
    const StationData *findCall(const std::string& call);
    
    /**
     * @brief 	Find a station in the station list given a station ID
     * @param 	id  The ID to find
     * @return	Returns a pointer to a StationData object if the ID was
     *	      	found. Otherwise a NULL-pointer is returned.
     */
    const StationData *findStation(int id);

    /**
     * @brief	Find stations from their mapping code
     * @param	stns This list is filled in by this function
     * @param	code The code to searh for
     * @param	exact \em true if it should be an exact match or else
     *                \em false
     *
     * Find stations matching the given code. For a description of how the
     * callsign to code mapping is done see @see EchoLink::StationData::code.
     */
    void findStationsByCode(std::vector<StationData> &stns,
		    const std::string& code, bool exact=true);
    
    /**
     * @brief A signal that is emitted when the registration status changes
     * @param status The new status
     */
    sigc::signal<void, StationData::Status> statusChanged;
    
    /**
     * @brief A signal that is emitted when the station list has been updated
     */
    sigc::signal<void> stationListUpdated;
    
    /**
     * @brief A signal that is emitted when an error occurs
     * @param msg The error message
     */
    sigc::signal<void, const std::string&> error;
    
  protected:
    
  private:
    typedef enum
    {
      CS_WAITING_FOR_START, CS_WAITING_FOR_COUNT, CS_WAITING_FOR_CALL,
      CS_WAITING_FOR_DATA,  CS_WAITING_FOR_ID,    CS_WAITING_FOR_IP,
      CS_WAITING_FOR_END,   CS_IDLE,  	      	  CS_WAITING_FOR_OK
    } ComState;
    
    static const int DIRECTORY_SERVER_PORT    	= 5200;
    static const int REGISTRATION_REFRESH_TIME  = 5 * 60 * 1000; // 5 minutes
    static const int CMD_TIMEOUT                = 120 * 1000; // 2 minutes
    
    ComState      	      com_state;
    std::vector<std::string>  the_servers;
    std::string       	      the_callsign;
    std::string       	      the_password;
    std::string       	      the_description;
    std::list<StationData>    the_links;
    std::list<StationData>    the_repeaters;
    std::list<StationData>    the_stations;
    std::list<StationData>    the_conferences;
    std::string       	      the_message;
    std::string       	      error_str;
    
    int       	      	      get_call_cnt;
    StationData       	      get_call_entry;
    std::list<StationData>    get_call_list;
    
    DirectoryCon *            ctrl_con;
    std::list<Cmd>    	      cmd_queue;
    StationData::Status       the_status;
    Async::Timer *    	      reg_refresh_timer;
    StationData::Status       current_status;
    bool      	      	      server_changed;
    Async::Timer *            cmd_timer;
    Async::IpAddress          bind_ip;
    
    Directory(const Directory&);
    Directory& operator =(const Directory&);
    
    void printBuf(const unsigned char *buf, int len);
    int handleCallList(char *buf, int len);
    
    void ctrlSockReady(bool is_ready);
    void ctrlSockConnected(void);
    void ctrlSockDisconnected(void);
    int ctrlSockDataReceived(void *ptr, unsigned len);
    void sendNextCmd(void);
    void addCmdToQueue(Cmd cmd);
    void setStatus(StationData::Status new_status);
    void createClientObject(void);
    void onRefreshRegistration(Async::Timer *timer);
    void onCmdTimeout(Async::Timer *timer);
    bool stationCodeEq(const StationData& stn, std::string code, bool exact);

};  /* class Directory */


std::ostream& operator<<(std::ostream& os, const StationData& station);


} /* namespace */


#endif /* ECHOLINK_DIRECTORY_INCLUDED */



/*
 * This file has not been truncated
 */

