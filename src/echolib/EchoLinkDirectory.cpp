/**
@file	 EchoLinkDirectory.cpp
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




/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <functional>

#include <cstdio>
#include <cerrno>
#include <cctype>
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

#include "EchoLinkDirectory.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
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

class Cmd
{
  public:
    typedef enum { OFFLINE, ONLINE, BUSY, GET_CALLS } Type;
    Type type;
    
    Cmd(Type type) : type(type) {}
};



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
 * Method:    Directory::Directory
 * Purpose:   Constructor
 * Input:     
 * Output:    None
 * Author:    Tobias Blomberg
 * Created:   2003-03-08
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
Directory::Directory(const string& server, const string& callsign,
    const string& password, const string& description)
  : com_state(CS_IDLE),       	      	      the_server(server),
    the_password(password),   	      	      the_description(""),
    error_str(""),    	      	      	      ctrl_con(0),
    the_status(StationData::STAT_OFFLINE),    reg_refresh_timer(0),
    current_status(StationData::STAT_OFFLINE),server_changed(false),
    cmd_timer(0)
{
  the_callsign.resize(callsign.size());
  transform(callsign.begin(), callsign.end(), the_callsign.begin(), ::toupper);
  
  setDescription(description);
  
  createClientObject();
  
  reg_refresh_timer = new Timer(REGISTRATION_REFRESH_TIME,
      Timer::TYPE_PERIODIC);
  reg_refresh_timer->expired.connect(
      slot(*this, &Directory::onRefreshRegistration));

} /* Directory::Directory */


Directory::~Directory(void)
{
  delete reg_refresh_timer;
  delete cmd_timer;
  delete ctrl_con;
} /* Directory::~Directory */


void Directory::makeOnline(void)
{
  the_status = StationData::STAT_ONLINE;
  addCmdToQueue(Cmd(Cmd::ONLINE));
} /* Directory::makeOnline */


void Directory::makeBusy()
{
  the_status = StationData::STAT_BUSY;
  addCmdToQueue(Cmd(Cmd::BUSY));
} /* Directory::makeBusy */


void Directory::makeOffline(void)
{
  the_status = StationData::STAT_OFFLINE;
  addCmdToQueue(Cmd(Cmd::OFFLINE));
} /* Directory::makeOffline */


void Directory::getCalls(void)
{
  if ((current_status == StationData::STAT_ONLINE) ||
      (current_status == StationData::STAT_BUSY))
  {
    list<Cmd>::const_iterator it;
    for (it = cmd_queue.begin(); it != cmd_queue.end(); ++it)
    {
      if (it->type == Cmd::GET_CALLS)
      {
	return;
      }
    }
    addCmdToQueue(Cmd(Cmd::GET_CALLS));
  }
  else
  {
    the_links.clear();
    the_repeaters.clear();
    the_conferences.clear();
    the_stations.clear();
    error("Trying to update the directory list while not registered with the "
      	  "directory server");
    //stationListUpdated();
  }
} /* Directory::getCalls */


void Directory::setServer(const string& server)
{
  server_changed = true;
  the_server = server;
} /* Directory::setServer */


void Directory::setCallsign(const string& callsign)
{
  the_callsign.resize(callsign.size());
  transform(callsign.begin(), callsign.end(), the_callsign.begin(), ::toupper);
  //the_callsign = callsign;
} /* Directory::setCall */


void Directory::setPassword(const string& password)
{
  the_password = password;
} /* Directory::setPassword */


void Directory::setDescription(const string& description)
{
  the_description = description;
  if (the_description.size() > MAX_DESCRIPTION_SIZE)
  {
    the_description.resize(MAX_DESCRIPTION_SIZE);
  }
} /* Directory::setDescription */


const StationData *Directory::findCall(const string& call)
{
  list<StationData> calls;
  list<StationData>::const_iterator iter;
  
  for (iter=the_links.begin(); iter!=the_links.end(); ++iter)
  {
    if (iter->callsign() == call)
    {
      return &(*iter);
    }
  }
  
  for (iter=the_repeaters.begin(); iter!=the_repeaters.end(); ++iter)
  {
    if (iter->callsign() == call)
    {
      return &(*iter);
    }
  }
  
  for (iter=the_conferences.begin(); iter!=the_conferences.end(); ++iter)
  {
    if (iter->callsign() == call)
    {
      return &(*iter);
    }
  }
  
  for (iter=the_stations.begin(); iter!=the_stations.end(); ++iter)
  {
    if (iter->callsign() == call)
    {
      return &(*iter);
    }
  }
  
  return 0;
  
} /* Directory::findCall */


const StationData *Directory::findStation(int id)
{
  list<StationData> calls;
  list<StationData>::const_iterator iter;
  
  for (iter=the_links.begin(); iter!=the_links.end(); ++iter)
  {
    if (iter->id() == id)
    {
      return &(*iter);
    }
  }
  
  for (iter=the_repeaters.begin(); iter!=the_repeaters.end(); ++iter)
  {
    if (iter->id() == id)
    {
      return &(*iter);
    }
  }
  
  for (iter=the_conferences.begin(); iter!=the_conferences.end(); ++iter)
  {
    if (iter->id() == id)
    {
      return &(*iter);
    }
  }
  
  for (iter=the_stations.begin(); iter!=the_stations.end(); ++iter)
  {
    if (iter->id() == id)
    {
      return &(*iter);
    }
  }
  
  return 0;
  
} /* Directory::findStation */


bool Directory::stationCodeEq(const StationData& stn, string code, bool exact)
{
  if (exact)
  {
    return (stn.code() == code);
  }
  else
  {
    const char *stn_code = stn.code().c_str();
    return (strstr(stn_code, code.c_str()) == stn_code);
  }
} /* Directory::stationCodeEq  */


void Directory::findStationsByCode(vector<StationData> &stns,
		const string& code, bool exact)
{
  list<StationData>::const_iterator iter;
  
  stns.clear();

  for (iter=the_links.begin(); iter!=the_links.end(); ++iter)
  {
    if (stationCodeEq(*iter, code, exact))
    {
      stns.push_back(*iter);
    }
  }
  
  for (iter=the_repeaters.begin(); iter!=the_repeaters.end(); ++iter)
  {
    if (stationCodeEq(*iter, code, exact))
    {
      stns.push_back(*iter);
    }
  }
  
  for (iter=the_conferences.begin(); iter!=the_conferences.end(); ++iter)
  {
    if (stationCodeEq(*iter, code, exact))
    {
      stns.push_back(*iter);
    }
  }
  
  for (iter=the_stations.begin(); iter!=the_stations.end(); ++iter)
  {
    if (stationCodeEq(*iter, code, exact))
    {
      stns.push_back(*iter);
    }
  }

} /* Directory::findStationsByCode  */


ostream& EchoLink::operator<<(ostream& os, const StationData& station)
{
  os  << setiosflags(ios::left)
      << setw(StationData::MAXCALL) << station.callsign().c_str()
      << setw(5) << station.statusStr().c_str()
      << setw(6) << station.time().c_str()
      << setw(30) << station.description().c_str()
      << setw(7) << station.id()
      << station.ip();
  return os;
} /* EchoLink::operator<< */


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
 * Method:    Directory::printBuf
 * Purpose:   Print the contents of "buf". Used for debugging.
 * Input:     buf - The buffer to print
 *    	      len - The length of the buffer to print
 * Output:    None
 * Author:    Tobias Blomberg
 * Created:   2003-03-08
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void Directory::printBuf(const unsigned char *buf, int len)
{
  for(int i=0; i<len; i++)
  {
    if (isprint(buf[i]))
    {
      fprintf(stderr, "%c", buf[i]);
    }
    else
    {
      fprintf(stderr, "<%02x>", buf[i]);
    }
  }
  fprintf(stderr, "\n");
} /* Directory::printBuf */


int Directory::handleCallList(char *buf, int len)
{
  int read_len = 0;

  switch (com_state)
  {
    case CS_WAITING_FOR_START:
    {
      if (len >= 4)
      {
	if (memcmp(buf, "@@@\n", 4) == 0)
	{
	  com_state = CS_WAITING_FOR_COUNT;
	  read_len = 4;
	}
	else
	{
	  fprintf(stderr, "Error in call list format (@@@ expected).\n");
	  com_state = CS_IDLE;
	}
      }
      break;
    }

    case CS_WAITING_FOR_COUNT:
    {
      char *nl = (char *)memchr(buf, '\n', len);
      if (nl != 0)
      {
	read_len = nl-buf+1;
	buf[read_len-1] = 0;
	get_call_cnt = atoi(buf);
	//printf("Number of calls to get: %d\n", get_call_cnt);
	if (get_call_cnt > 0)
	{
	  get_call_list.clear();
	  the_message = "";
	  com_state = CS_WAITING_FOR_CALL;
	}
	else
	{
	  com_state = CS_WAITING_FOR_END;
	}
      }
      break;
    }

    case CS_WAITING_FOR_CALL:
    {	
      char *nl = (char *)memchr(buf, '\n', len);
      if (nl != 0)
      {
	read_len = nl-buf+1;
	buf[read_len-1] = 0;
	get_call_entry.clear();
	get_call_entry.setCallsign(buf);
	//printf("Station call: %s\n", get_call_callsign);
	com_state = CS_WAITING_FOR_DATA;
      }
      break;
    }

    case CS_WAITING_FOR_DATA:
    {
      char *nl = (char *)memchr(buf, '\n', len);
      if (nl != 0)
      {
	read_len = nl-buf+1;
	buf[read_len-1] = 0;
	get_call_entry.setData(buf);
	//printf("Station data: %s\n", get_call_data);
	com_state = CS_WAITING_FOR_ID;
      }
      break;
    }

    case CS_WAITING_FOR_ID:
    {
      char *nl = (char *)memchr(buf, '\n', len);
      if (nl != 0)
      {
	read_len = nl-buf+1;
	buf[read_len-1] = 0;
	get_call_entry.setId(atoi(buf));
	//printf("Station id: %s\n", get_call_id);
	com_state = CS_WAITING_FOR_IP;
      }
      break;
    }

    case CS_WAITING_FOR_IP:
    {
      char *nl = (char *)memchr(buf, '\n', len);
      if (nl != 0)
      {
	read_len = nl-buf+1;
	buf[read_len-1] = 0;
	get_call_entry.setIp(IpAddress(buf));
	//printf("Station ip: %s\n", get_call_ip);
	
	/*
	if (strlen(get_call_entry.callsign().c_str()) == 1)
	{
	  the_message += get_call_entry.description() + "\n";
	}
	else if (strchr(get_call_entry.callsign().c_str(), ' ') != 0)
	{
	  error_str = get_call_entry.callsign();
	}
	*/
	
	if (get_call_entry.callsign() == ".")
	{
	  com_state = CS_WAITING_FOR_CALL;
	  break;
	}
	
	if (get_call_entry.callsign() == " ")
	{
	  the_message += get_call_entry.description() + "\n";
	}
	else
	{
      	  get_call_list.push_back(get_call_entry);
	}

	if (--get_call_cnt <= 0)
	{
	  com_state = CS_WAITING_FOR_END;
	}
	else
	{
	  com_state = CS_WAITING_FOR_CALL;
	}
      }
      break;
    }

    case CS_WAITING_FOR_END:
    {
      if (len >= 3)
      {
	if (memcmp(buf, "+++", 3) == 0)
	{
	  //printf("End received!\n");
	  the_links.clear();
	  the_repeaters.clear();
	  the_conferences.clear();
	  the_stations.clear();
	  list<StationData>::const_iterator it;
	  for (it = get_call_list.begin(); it != get_call_list.end(); ++it)
	  {
	    if (strstr(it->callsign().c_str(), "-L"))
	    {
	      the_links.push_back(*it);
	    }
	    else if (strstr(it->callsign().c_str(), "-R"))
	    {
	      the_repeaters.push_back(*it);
	    }
	    else if (strstr(it->callsign().c_str(), "*"))
	    {
	      the_conferences.push_back(*it);
	    }
	    else
	    {
	      the_stations.push_back(*it);
	    }
	  }
	  get_call_list.clear();
	  com_state = CS_IDLE;
	  read_len = 3;
	}
	else
	{
	  fprintf(stderr, "Error in call list format (+++ expected).\n");
	  com_state = CS_IDLE;
	}
      }
      break;
    }

    case CS_IDLE:
      break;
    
    default:
      fprintf(stderr, "Illegal state in method handleCallList\n");
      assert(0);
      break;
  }
  
  return read_len;
  
} /* Directory::handleCallList */


void Directory::ctrlSockConnected(void)
{
  assert(!cmd_queue.empty());
  
  Cmd cmd = cmd_queue.front();
  string cmdstr;
  switch (cmd.type)
  {
    case Cmd::OFFLINE:
    {
      cmdstr = "l" + the_callsign + "\254\254" + the_password + 
	  "\015OFF-V3.40\015" + the_description + "\015";
      break;
    }
      
    case Cmd::ONLINE:
    {
      time_t t = time(NULL);
      struct tm *tm = localtime(&t);
      char local_time_str[6];
      strftime(local_time_str, 6, "%H:%M", tm);
      cmdstr = "l" + the_callsign + "\254\254" + the_password +
	  "\015ONLINE3.38(" + local_time_str + ")\015" + the_description +
	  "\015";
      break;
    }
      
    case Cmd::BUSY:
    {
      time_t t = time(NULL);
      struct tm *tm = localtime(&t);
      char local_time_str[6];
      strftime(local_time_str, 6, "%H:%M", tm);
      cmdstr = "l" + the_callsign + "\254\254" + the_password +
	  "\015BUSY3.40(" + local_time_str + ")\015" + the_description + "\015";
      break;
    }
      
    case Cmd::GET_CALLS:
    {
      cmdstr = "s";
      break;
    }
      
  }
  
  //cerr << "Connected. Writing: ";
  //printBuf(reinterpret_cast<const unsigned char *>(cmdstr.c_str()), cmdstr.size());
  ctrl_con->write(cmdstr.c_str(), cmdstr.size());
  //cerr << "Write returned: " << ret << endl;
} /* Directory::ctrlSockConnected */


int Directory::ctrlSockDataReceived(TcpConnection *con, void *ptr, int len)
{
  char *buf = static_cast<char *>(ptr);
  size_t tot_read_len = 0;
  size_t read_len = 0;

  //cerr << "Data received: ";
  //printBuf(reinterpret_cast<unsigned char *>(buf), len);
  
  do
  {
    read_len = 0;
    
    if (com_state == CS_WAITING_FOR_OK)
    {
      if (len >= 2)
      {
	if (memcmp(buf, "OK", 2) == 0)
	{
	  switch (cmd_queue.front().type)
	  {
	    case Cmd::OFFLINE:
	      setStatus(StationData::STAT_OFFLINE);
	      break;

	    case Cmd::ONLINE:
	      setStatus(StationData::STAT_ONLINE);
	      break;

	    case Cmd::BUSY:
	      setStatus(StationData::STAT_BUSY);
	      break;

	    default:
	      break;
	  }
	  //read_len = 2;
	}
	else
	{
	  fprintf(stderr, "Unexpected reply from directory server "
	      "(waiting for OK): ");
	  printBuf(reinterpret_cast<unsigned char *>(buf), len);
	  setStatus(StationData::STAT_UNKNOWN);
	}
	//printBuf(reinterpret_cast<unsigned char *>(buf), len);
	read_len = len;
	ctrl_con->disconnect();
	cmd_queue.pop_front();
	com_state = CS_IDLE;
	sendNextCmd();
      }
    }
    else if (com_state != CS_IDLE) 	// Waiting for station list
    {
      read_len = handleCallList(buf, len);
      if (com_state == CS_IDLE)
      {
      	//if (read_len < len)
	//{
	  //buf += read_len;
	  //len -= read_len;
	  //printBuf(reinterpret_cast<unsigned char *>(buf), len);
	//}
	read_len = len;
	ctrl_con->disconnect();
	if (!error_str.empty())
	{
	  error(error_str);
	}
	else
	{
	  stationListUpdated();
	}
	cmd_queue.pop_front();
	sendNextCmd();
      }
    }
    
    tot_read_len += read_len;
    buf += read_len;
    len -= read_len;
  } while (read_len > 0);
    
  return tot_read_len;
  
} /* Directory::ctrlSockDataReceived */


void Directory::ctrlSockDisconnected(TcpConnection *con,
      	Async::TcpClient::DisconnectReason reason)
{
  switch (reason)
  {
    case Async::TcpClient::DR_HOST_NOT_FOUND:
      error("Directory server host \"" + the_server + "\" not found\n");
      break;
    
    case Async::TcpClient::DR_REMOTE_DISCONNECTED:
      error("The directory server closed the connection before all data was "
	  "received\n");
      break;
      
    case Async::TcpClient::DR_SYSTEM_ERROR:
      error(string("Directory server communications error: ") + strerror(errno));
      break;
      
    case Async::TcpClient::DR_RECV_BUFFER_OVERFLOW:
      error("Directory server receiver buffer overflow!\n");
      break;
    
    case Async::TcpClient::DR_ORDERED_DISCONNECT:
      break;
  }
  
  assert(!cmd_queue.empty());

  switch (cmd_queue.front().type)
  {
    case Cmd::OFFLINE:
    case Cmd::ONLINE:
    case Cmd::BUSY:
      setStatus(StationData::STAT_UNKNOWN);
      break;
    
    case Cmd::GET_CALLS:
      break;
  }
  
  cmd_queue.pop_front();
  com_state = CS_IDLE;
  sendNextCmd();
  
} /* Directory::ctrlSockDisconnected */


void Directory::sendNextCmd(void)
{
  //cerr << "Directory::sendNextCmd\n";
  delete cmd_timer;
  cmd_timer = 0;

  if (cmd_queue.empty())
  {
    return;
  }
  
  cmd_timer = new Timer(CMD_TIMEOUT);
  cmd_timer->expired.connect(slot(*this, &Directory::onCmdTimeout));

  if (cmd_queue.front().type == Cmd::GET_CALLS)
  {
    error_str = "";
    com_state = CS_WAITING_FOR_START;
  }
  else
  {
    com_state = CS_WAITING_FOR_OK;
  }
  if (server_changed)
  {
    server_changed = false;
    delete ctrl_con;
    ctrl_con = 0;
    createClientObject();
  }
  //cerr << "Connecting...\n";
  ctrl_con->connect();
  
} /* Directory::sendNextCmd */


void Directory::addCmdToQueue(Cmd cmd)
{
  cmd_queue.push_back(cmd);
  if (com_state == CS_IDLE)
  {
    sendNextCmd();
  }
} /* Directory::addCmdToQueue */


void Directory::setStatus(StationData::Status new_status)
{
  if (new_status != current_status)
  {
    current_status = new_status;
    statusChanged(current_status);
  }
} /* Directory::setStatus */


void Directory::createClientObject(void)
{
  ctrl_con = new Async::TcpClient(the_server, DIRECTORY_SERVER_PORT);
  ctrl_con->connected.connect(slot(*this, &Directory::ctrlSockConnected));
  ctrl_con->dataReceived.connect(slot(*this, &Directory::ctrlSockDataReceived));
  ctrl_con->disconnected.connect(slot(*this, &Directory::ctrlSockDisconnected));
} /* Directory::createClientObject */


void Directory::onRefreshRegistration(Timer *timer)
{
  //printf("Directory::onRefreshRegistration: cmds=%d  com_state=%d\n",
	//cmd_queue.size(), com_state);

  if (the_status == StationData::STAT_ONLINE)
  {
    makeOnline();
  }
  else if (the_status == StationData::STAT_BUSY)
  {
    makeBusy();
  }
} /* Directory::onRefreshRegistration */


void Directory::onCmdTimeout(Timer *timer)
{
  error("Command timeout while communicating to the directory server");
  //cerr << "Directory::onCmdTimeout: Disconnecting...\n";
  ctrl_con->disconnect();

  assert(!cmd_queue.empty());

  switch (cmd_queue.front().type)
  {
    case Cmd::OFFLINE:
    case Cmd::ONLINE:
    case Cmd::BUSY:
      setStatus(StationData::STAT_UNKNOWN);
      break;
    
    case Cmd::GET_CALLS:
      break;
  }

  cmd_queue.pop_front();
  com_state = CS_IDLE;
  sendNextCmd();
} /* Directory::onCmdTimeout */



/*
 * This file has not been truncated
 */

