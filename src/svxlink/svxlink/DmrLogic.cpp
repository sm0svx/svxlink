/**
@file	 DmrLogic.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2017-02-12

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003-2017 Tobias Blomberg / SM0SVX

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

#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <string>
#include <ctime>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/


#include <AsyncUdpSocket.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "DmrLogic.h"
#include "sha256.h"
#include "version/SVXLINK.h"


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

DmrLogic::DmrLogic(Async::Config& cfg, const std::string& name)
  : LogicBase(cfg, name), m_state(DISCONNECTED), m_udp_sock(0), 
    m_auth_key("passw0rd"),dmr_host(""), dmr_port(62032), 
    m_callsign("N0CALL"), m_id(""), m_ping_timer(60000, 
    Timer::TYPE_PERIODIC, false),m_slot1(false),m_slot2(false)
{
} /* DmrLogic::DmrLogic */


DmrLogic::~DmrLogic(void)
{
  delete m_udp_sock;
  m_ping_timer = 0;
  delete dns;
} /* DmrLogic::~DmrLogic */


bool DmrLogic::initialize(void)
{
  if (!cfg().getValue(name(), "HOST", dmr_host))
  {
    cerr << "*** ERROR: " << name() << "/HOST missing in configuration" << endl;
    return false;
  }

  cfg().getValue(name(), "PORT", dmr_port);

  if (!cfg().getValue(name(), "CALLSIGN", m_callsign))
  {
    cerr << "*** ERROR: " << name() << "/CALLSIGN missing in configuration"
         << endl;
    return false;
  }

  string m_t_id;
  if (!cfg().getValue(name(), "ID", m_t_id))
  {
    cerr << "*** ERROR: " << name() << "/ID missing in configuration"
         << endl;
    return false;
  }

  if (m_t_id.length() < 6 || m_t_id.length() > 7)
  {
    cerr << "*** ERROR: " << name() << "/ID is wrong, must have 6 or 7 digits,"
         << "e.g. ID=2620001" << endl;
    return false;
  }
  stringstream ss;
  ss << std::setfill('0') << std::setw(8) << std::hex << atoi(m_t_id.c_str());
  m_id = ss.str();
  std::transform(m_id.begin(), m_id.end(), m_id.begin(), ::toupper);

  if (!cfg().getValue(name(), "AUTH_KEY", m_auth_key))
  {
    cerr << "*** ERROR: " << name() << "/AUTH_KEY missing in configuration"
         << endl;
    return false;
  }

  if (!cfg().getValue(name(), "RX_FREQU", m_rxfreq))
  {
    cerr << "*** ERROR: " << name() << "/RX_FREQU missing in configuration"
         << endl;
    return false;
  }
  if (m_rxfreq.length() != 9)
  {
    cerr << "*** ERROR: " << name() << "/RX_FREQU wrong length: " << m_rxfreq
         << endl;
    return false;      
  }

  if (!cfg().getValue(name(), "TX_FREQU", m_txfreq))
  {
    cerr << "*** ERROR: " << name() << "/TX_FREQU missing in configuration"
         << endl;
    return false;
  }
  if (m_txfreq.length() != 9)
  {
    cerr << "*** ERROR: " << name() << "/TX_FREQU wrong length: " << m_txfreq
         << endl;
    return false;      
  }
  
  if (!cfg().getValue(name(), "POWER", m_power))
  {
    m_power = "01";
  }
  if (m_power.length() != 2)
  {
    cerr << "*** ERROR: " << name() << "/POWER wrong length: " << m_power
         << endl;
    return false;      
  }

  if (!cfg().getValue(name(), "COLORCODE", m_color))
  {
    m_color = "01";
  }
  if (m_color.length() != 2 || atoi(m_color.c_str()) <= 0)
  {
    cerr << "*** ERROR: " << name() << "/COLORCODE wrong: " << m_color
         << endl;
    return false;
  }
  
  if (!cfg().getValue(name(), "LATITUDE", m_lat))
  {
    cerr << "*** ERROR: " << name() << "/LATITUDE not set."
         << endl;
    return false;
  }
  if (m_lat.length() != 8)
  {
    cerr << "*** ERROR: " << name() << "/LATITUDE wrong length: " 
         << m_lat << endl;
    return false;
  }

  if (!cfg().getValue(name(), "LONGITUDE", m_lon))
  {
    cerr << "*** ERROR: " << name() << "/LONGITUDE not set."
         << endl;
    return false;
  }
  if (m_lon.length() != 9)
  {
    cerr << "*** ERROR: " << name() << "/LONGITUDE wrong length: " 
         << m_lon << endl;
    return false;
  }
  
  if (!cfg().getValue(name(), "HEIGHT", m_height))
  {
    m_height = "001";
  }
  if (m_height.length() != 3)
  {
    cerr << "*** ERROR: " << name() << "/HEIGHT wrong: " << m_height
         << endl;
    return false;      
  }
  
  if (!cfg().getValue(name(), "LOCATION", m_location))
  {
    m_location = "none";
  }
  if (m_location.length() > 20)
  {
    cerr << "*** ERROR: " << name() << "/LOCATION to long: " << m_location
         << endl;
    return false;      
  }
  while (m_location.length() < 20)
  {
    m_location += ' ';
  }
  
  if (!cfg().getValue(name(), "DESCRIPTION", m_description))
  {
    m_description = "none";
  }
  if (m_description.length() > 20)
  {
    cerr << "*** ERROR: " << name() << "/DESCRIPTION to long (>20 chars)" 
         << endl;
    return false;      
  }
  while (m_description.length() < 20)
  {
    m_description += ' ';
  }
  
    // configure the time slots
  string slot;
  if (cfg().getValue(name(), "SLOT1", slot))
  {
    m_slot1 = true;
  }
  if (cfg().getValue(name(), "SLOT2", slot))
  {
    m_slot2 = true;
  }
  
  // the url of the own node
  if (!cfg().getValue(name(), "URL", m_url))
  {
    m_url = "http://svxlink.de";
  }
  if (m_url.length() > 124)
  {
    cerr << "*** ERROR: " << name() << "/URL to long: " 
         << m_url << endl;
    return false;      
  }
  while (m_url.length() < 124)
  {
    m_url += ' ';
  }
  
  m_swid += "linux:SvxLink v";
  m_swid += SVXLINK_VERSION;
  while (m_swid.length() < 40)
  {
    m_swid += ' ';
  }
  
  m_pack = "08032017                                ";
  

  if (!LogicBase::initialize())
  {
    return false;
  }

  m_ping_timer.setEnable(false);
  m_ping_timer.expired.connect(
     mem_fun(*this, &DmrLogic::pingHandler));

  cout << "connecting to " << dmr_host << endl;
  connect();

  return true;
} /* DmrLogic::initialize */



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

void DmrLogic::connect(void)
{

  if (ip_addr.isEmpty())
  {
    cout << "looking up dns " << endl;
    dns = new DnsLookup(dmr_host);
    dns->resultsReady.connect(mem_fun(*this, &DmrLogic::dnsResultsReady));
    return;
  }

  cout << name() << ": create UDP socket on port " << dmr_port << endl;

  delete m_udp_sock;
  m_udp_sock = new Async::UdpSocket();
  m_udp_sock->dataReceived.connect(
      mem_fun(*this, &DmrLogic::onDataReceived));

  // init Login to Server
  cout << "### login to " << dmr_host << endl;
  char msg[13];
  sprintf(msg, "RPTL%s", m_id.c_str());
  sendMsg(msg);
  
  m_state = CONNECTING;
  m_ping_timer.setEnable(true);

} /* DmrLogic::onConnected */


void DmrLogic::dnsResultsReady(DnsLookup& dns_lookup)
{
  vector<IpAddress> result = dns->addresses();

  delete dns;
  dns = 0;

  if (result.empty() || result[0].isEmpty())
  {
    ip_addr.clear();
    return;
  }

  ip_addr = result[0];
  connect();

} /* DmrLogic::dnsResultsReady */


void DmrLogic::flushEncodedAudio(void)
{
  //cout << "### " << name() << ": DmrLogic::flushEncodedAudio" << endl;
} /* DmrLogic::sendEncodedAudio */


void DmrLogic::onDataReceived(const IpAddress& addr, uint16_t port,
                                         void *buf, int count)
{
  cout << "### " << name() << ": DmrLogic::onDataReceived: addr="
       << addr << " port=" << port << " count=" << count << endl;

  string token(reinterpret_cast<const char *>(buf));

  size_t found;

   // look for ACK message from server
  if ((found = token.find("MSTNAK")) != std::string::npos)
  {
    cout << "*** ERROR: MSTNAK from server " << addr << endl;
    return;
  }

    // got MSTACK from server
  if ((found = token.find("RPTACK")) != std::string::npos)
  {
    if (m_state == CONNECTING)
    {
      token.erase(0,6);
      const char *pass = token.c_str();
      //char m_random_id[9];
      //sprintf(m_random_id, "%02X%02X%02X%02X", t[0] & 0xff, (t[1]>>8) & 0xff,
      //                          (t[2]>>16) & 0xff, (t[3]>>24) & 0xff);
      cout << pass << endl;
      authPassphrase(pass);
      return;
    }
    
    // passphrase accepted by server
    if (m_state == WAITING_PASS_ACK)
    {
      m_state = AUTHENTICATED;
      cout << "--- authentified, sending configuration" << endl;
      sendConfiguration();
    }
  }

    // server sent a ping, requesting pong
  if ((found = token.find("MSTPING")) != std::string::npos)
  {
    cout << "Server sent a MSTPING, requesting MSTPONG" << endl;
    sendPong();
  }

    // server sent a pong
  if ((found = token.find("MSTPONG")) != std::string::npos)
  {
    cout << "Server sent a MSTPONG, OK :)" << endl;
  }
  
    // server sent close command
  if ((found = token.find("MSTCL")) != std::string::npos)
  {
    cout << "Server sent a MSTTCL, closing session :(" << endl;
    sendCloseMessage();
  }
  
    // server sent data command
  if ((found = token.find("DMRD")) != std::string::npos)
  {
    cout << "Server sent a DMRD" << endl;
    handleDataMessage(token.erase(0,4));
  }
} /* DmrLogic::udpDatagramReceived */


void DmrLogic::authPassphrase(const char *pass)
{
  std::stringstream resp;
  resp << pass << m_auth_key;

  std::string p_msg = "RPTK";
  p_msg += m_id;
  cout << "P-Message: " << p_msg << endl;
  p_msg += sha256(resp.str());

  cout << "### sending: " << p_msg << endl;
  sendMsg(p_msg);
  m_state = WAITING_PASS_ACK;
} /* DmrLogic::authPassphrase */


void DmrLogic::sendMsg(std::string msg)
{

  if (ip_addr.isEmpty())
  {
    if (!dns)
    {
      dns = new DnsLookup(dmr_host);
      dns->resultsReady.connect(mem_fun(*this, &DmrLogic::dnsResultsReady));
    }
    return;
  }

  if (m_udp_sock == 0)
  {
    cerr << "*** ERROR: No Udp socket available" << endl;
    return;
  }

  const char *dmr_packet = msg.c_str();

  cout << "### sending udp packet: " << msg << " to " << ip_addr.toString()
       << endl;
  m_udp_sock->write(ip_addr, dmr_port, dmr_packet, sizeof(dmr_packet));

} /* DmrLogic::sendUdpMsg */


void DmrLogic::reconnect(Timer *t)
{
  cout << "### Reconnecting to DMR server\n";
} /* DmrLogic::reconnect */


void DmrLogic::allEncodedSamplesFlushed(void)
{
} /* DmrLogic::allEncodedSamplesFlushed */


void DmrLogic::pingHandler(Async::Timer *t)
{
  // cout << "sending PING\n";
  sendPing();
} /* DmrLogic::heartbeatHandler */


void DmrLogic::sendPing(void)
{
  std::string p_msg = "MSTPING";
  p_msg += m_id;
  sendMsg(p_msg);

  m_ping_timer.reset();
} /* DmrLogic::sendPing */


void DmrLogic::sendPong(void)
{
  std::string p_msg = "MSTPONG";
  p_msg += m_id;
  sendMsg(p_msg);
} /* DmrLogic::sendPong */


void DmrLogic::sendCloseMessage(void)
{
  std::string p_msg = "RPTCL";
  p_msg += m_id;
  sendMsg(p_msg);
} /* DmrLogic::sendCloseMessage */


void DmrLogic::sendConfiguration(void)
{
  std::string p_msg = "RPTC";
  p_msg += m_callsign;
  p_msg += m_id;
  p_msg += m_rxfreq;
  p_msg += m_txfreq;
  p_msg += m_power;
  p_msg += m_color;
  p_msg += m_lat;
  p_msg += m_lon;
  p_msg += m_height;
  p_msg += m_location;
  p_msg += m_description;
  p_msg += m_url;
  p_msg += m_swid;
  p_msg += m_pack;
  cout << "--- sending configuration: " << p_msg << endl;
  sendMsg(p_msg);  
}


void DmrLogic::handleDataMessage(std::string dmessage)
{
  char const *dmsg = dmessage.c_str();
  int m_sid = (int)&dmsg[0]; // squence number
  
  if (++seqId != m_sid)
  {
    cout << "WARNING: Wrong sequence number " << seqId << "!=" 
         << m_sid << endl;
    seqId = m_sid;
  }

  //int srcId = 
  //int dstId = 
  //int rptId =
  //int slot = &dmsg[12] & 0x80;
  //int calltype = &dmsg[12] & 0x40;
  //int frametype = &dmsg[12] & 0x30;
  //int datatype = 
  //int voiceseq =
  //int streamid = 
  //int dmrdata =
  
} /* DmrLogic::handleDataMessage */

/*
 * This file has not been truncated
 */

