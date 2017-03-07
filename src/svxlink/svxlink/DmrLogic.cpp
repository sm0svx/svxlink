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
#include <iomanip>
#include <cstring>
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
  : LogicBase(cfg, name), m_udp_sock(0), m_auth_key("passw0rd"),dmr_host(""),
    dmr_port(62032), m_callsign("N0CALL"), m_id(""),
    m_ping_timer(60000, Timer::TYPE_PERIODIC, false)
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

  if (!cfg().getValue(name(), "ID", m_id))
  {
    cerr << "*** ERROR: " << name() << "/ID missing in configuration"
         << endl;
    return false;
  }
  if (m_id.length() != 8)
  {
    cerr << "*** ERROR: " << name() << "/ID is wrong, must have 8 digits, "
         << "e.g. ID=00123456" << endl;
    return false;
  }

  if (!cfg().getValue(name(), "AUTH_KEY", m_auth_key))
  {
    cerr << "*** ERROR: " << name() << "/AUTH_KEY missing in configuration"
         << endl;
    return false;
  }
  if (m_auth_key == "Change this key now!")
  {
    cerr << "*** ERROR: You must change " << name() << "/AUTH_KEY from the "
            "default value" << endl;
    return false;
  }

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

  cout << token << endl;

   // look for ACK message from server
  if ((found = token.find("MSTNAK")) != std::string::npos)
  {
    cout << "*** ERROR: MSTNAK from server " << addr << endl;
    return;
  }

    // got MSTACK from server
  if ((found = token.find("RPTACK")) != std::string::npos)
  {
    token.erase(0,6);
    const char *t = token.c_str();
    char m_random_id[9];
    sprintf(m_random_id, "%02X%02X%02X%02X", t[0]&0xff, (t[1]>>8) & 0xff,
                                     (t[2]>>16) & 0xff, (t[3]>>24) & 0xff);
    cout << m_random_id << endl;
    m_state = WAITING_PASS_ACK;
    authPassphrase((string)m_random_id);
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

} /* DmrLogic::udpDatagramReceived */


void DmrLogic::authPassphrase(std::string pass)
{
  std::stringstream resp;
  resp << pass << m_auth_key;

  std::string p_msg = "RPTK";
  p_msg += m_id;
  p_msg += sha256(resp.str());

  cout << "### sending: " << p_msg << endl;
  sendMsg(p_msg);
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

  cout << "### sending upd packet: " << msg << " to " << ip_addr.toString()
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
  // cout << "seiding PING\n";
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


/*
 * This file has not been truncated
 */

