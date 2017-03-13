/**
@file	 RewindLogic.cpp
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

#include "RewindLogic.h"
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

RewindLogic::RewindLogic(Async::Config& cfg, const std::string& name)
  : LogicBase(cfg, name), m_state(DISCONNECTED), m_udp_sock(0),
    m_auth_key("passw0rd"),Rewind_host(""), Rewind_port(62032),
    m_callsign("N0CALL"), m_id(""), m_ping_timer(10000,
    Timer::TYPE_PERIODIC, false), m_slot1(false), m_slot2(false)
{
} /* RewindLogic::RewindLogic */


RewindLogic::~RewindLogic(void)
{
  delete m_udp_sock;
  m_ping_timer = 0;
  delete dns;
  delete m_logic_con;
} /* RewindLogic::~RewindLogic */


bool RewindLogic::initialize(void)
{
  if (!cfg().getValue(name(), "HOST", Rewind_host))
  {
    cerr << "*** ERROR: " << name() << "/HOST missing in configuration"
         << endl;
    return false;
  }

  cfg().getValue(name(), "PORT", Rewind_port);

  if (!cfg().getValue(name(), "CALLSIGN", m_callsign))
  {
    cerr << "*** ERROR: " << name() << "/CALLSIGN missing in configuration"
         << endl;
    return false;
  }
  while (m_callsign.length() < 8)
  {
    m_callsign += ' ';
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
         << ", e.g. RX_FREQU=430500000"  << endl;
    return false;
  }
  if (m_rxfreq.length() != 9)
  {
    cerr << "*** ERROR: " << name() << "/RX_FREQU wrong length: " << m_rxfreq
         << ", e.g. RX_FREQU=430500000" << endl;
    return false;
  }

  if (!cfg().getValue(name(), "TX_FREQU", m_txfreq))
  {
    cerr << "*** ERROR: " << name() << "/TX_FREQU missing in configuration"
         << ", e.g. TX_FREQU=430500000" << endl;
    return false;
  }
  if (m_txfreq.length() != 9)
  {
    cerr << "*** ERROR: " << name() << "/TX_FREQU wrong length: " << m_txfreq
         << ", e.g. TX_FREQU=430500000" << endl;
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

  m_swid += "linux:SvxLink v";
  m_swid += SVXLINK_VERSION;

  m_ping_timer.setEnable(false);
  m_ping_timer.expired.connect(
     mem_fun(*this, &RewindLogic::pingHandler));

  // create the Rewind recoder device, DV3k USB stick or DSD lib
  string m_ambe_handler;
  if (!cfg().getValue(name(), "AMBE_HANDLER", m_ambe_handler))
  {
    cerr << "*** ERROR: " << name() << "/AMBE_HANDLER not valid, must be"
         << " DV3k or SwDsd" << endl;
    return false;
  }

  m_logic_con = Async::AudioRecoder::create(m_ambe_handler);

  if (m_logic_con == 0)
  {
    cerr << "*** ERROR: Failed to initialize Rewind recoder" << endl;
    return false;
  }

  m_logic_con->writeEncodedSamples.connect(
               mem_fun(*this, &RewindLogic::sendEncodedAudio));
  m_logic_con->flushEncodedSamples.connect(
               mem_fun(*this, &RewindLogic::flushEncodedAudio));
  m_logic_con->allDecodedSamplesFlushed.connect(
      mem_fun(*this, &RewindLogic::allEncodedSamplesFlushed));

  // sending options to audio decoder
  string opt_prefix(m_logic_con->name());
  opt_prefix += "_";
  list<string> names = cfg().listSection(name());
  list<string>::const_iterator nit;
  for (nit=names.begin(); nit!=names.end(); ++nit)
  {
    if ((*nit).find(opt_prefix) == 0)
    {
      string opt_value;
      cfg().getValue(name(), *nit, opt_value);
      string opt_name((*nit).substr(opt_prefix.size()));
      m_logic_con->setOption(opt_name, opt_value);
    }
  }

  if (!LogicBase::initialize())
  {
    return false;
  }

  connect();
  return true;
} /* RewindLogic::initialize */



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

void RewindLogic::connect(void)
{

  if (ip_addr.isEmpty())
  {
    dns = new DnsLookup(Rewind_host);
    dns->resultsReady.connect(mem_fun(*this, &RewindLogic::dnsResultsReady));
    return;
  }

  cout << name() << ": create UDP socket on port " << Rewind_port << endl;

  delete m_udp_sock;
  m_udp_sock = new Async::UdpSocket();
  m_udp_sock->dataReceived.connect(
      mem_fun(*this, &RewindLogic::onDataReceived));

   // session initiated by a simple REWIND_TYPE_KEEP_ALIVE message
  sendPing();

  m_state = CONNECTING;
  m_ping_timer.setEnable(true);

} /* RewindLogic::onConnected */


void RewindLogic::dnsResultsReady(DnsLookup& dns_lookup)
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
} /* RewindLogic::dnsResultsReady */


void RewindLogic::sendEncodedAudio(const void *buf, int count)
{
  //cout << "### " << name() << ": ReflectorLogic::sendEncodedAudio: count="
  //     << count << endl;
/*  if (m_flush_timeout_timer.isEnabled())
  {
    m_flush_timeout_timer.setEnable(false);
  }
  sendUdpMsg(MsgUdpAudio(buf, count));*/
} /* RewindLogic::sendEncodedAudio */


void RewindLogic::flushEncodedAudio(void)
{
  //cout << "### " << name() << ": RewindLogic::flushEncodedAudio" << endl;
} /* RewindLogic::sendEncodedAudio */


void RewindLogic::onDataReceived(const IpAddress& addr, uint16_t port,
                                         void *buf, int count)
{
  cout << "### " << name() << ": RewindLogic::onDataReceived: addr="
       << addr << " port=" << port << " count=" << count << endl;

  rd = reinterpret_cast<RewindData*>(buf);
  
  switch rd->type
  {
    case REWIND_TYPE_REPORT:
      return;

    case REWIND_TYPE_CHALLENGE:
      cout << "--- Authentication" << endl;
      authenticate(m_auth_key);
      return;
      
    case REWIND_TYPE_CLOSE:
      cout << "*** Disconnect request received." << endl;
      m_state = DISCONNECTED;
      m_ping_timer.setEnable(false);
      return;
      
    case REWIND_TYPE_KEEP_ALIVE:
      sendPing();
      return;
    
    case REWIND_TYPE_FAILURE_CODE:
      cout << "*** ERROR: sourceCall or destinationCall could not be "
           << "resolved during processing." << endl;
      return;
      
    default:
      cout << "*** Unknown data received" << endl;
  }
} /* RewindLogic::udpDatagramReceived */


void RewindLogic::authenticate(const string pass)
{
  RewindData *srd;
  srd->type = htole16(REWIND_TYPE_AUTHENTICATION);
  srd->flags = htole16(REWIND_FLAG_NONE);
  srd->number = htole32(++sequenceNumber);
  srd->length = htole16(SHA256_DIGEST_LENGTH);
  srd->data = 
   
  std::string p_msg = sha256(pass);

  cout << "### sending: " << p_msg << endl;
  sendMsg(srd, sizeof(struct RewindData) + SHA256_DIGEST_LENGTH);
  m_state = WAITING_PASS_ACK;
} /* RewindLogic::authPassphrase */


void RewindLogic::sendMsg(RewindData *rd, size_t len)
{

  if (ip_addr.isEmpty())
  {
    if (!dns)
    {
      dns = new DnsLookup(Rewind_host);
      dns->resultsReady.connect(mem_fun(*this, &RewindLogic::dnsResultsReady));
    }
    return;
  }

  if (m_udp_sock == 0)
  {
    cerr << "*** ERROR: No Udp socket available" << endl;
    return;
  }

  cout << "### sending udp packet to " << ip_addr.toString()
       << ":" << rewind_port << " size=" << length << endl;

  m_udp_sock->write(ip_addr, rewind_port, rd, length);
} /* RewindLogic::sendUdpMsg */


void RewindLogic::reconnect(Timer *t)
{
  cout << "### Reconnecting to Rewind server\n";
} /* RewindLogic::reconnect */


void RewindLogic::allEncodedSamplesFlushed(void)
{
} /* RewindLogic::allEncodedSamplesFlushed */


void RewindLogic::pingHandler(Async::Timer *t)
{
  // cout << "PING timeout\n";
  sendKeepAlive();
} /* RewindLogic::heartbeatHandler */


void RewindLogic::sendVersionData(void)
{
  RewindVersionData *vd;
  RewindData *rd;
  
   // the appication ID number provided by 
   // Brandmeister sysops
  vd->number = strtoul(m_t_id.c_str()); 
  vd->service = REWIND_SERVICE_SIMPLE_APPLICATION;
  vd->description = m_swid;

  size_t len = sizeof(struct RewindVersionData) + m_swid.length();

  rd->type = htole16(REWIND_TYPE_KEEP_ALIVE);
  rd->flags = htole16(REWIND_FLAG_NONE);
  rd->number = htole32(++sequenceNumber);
  rd->length = htole16(len);
  len += sizeof(struct RewindData);
  
  sendMsg(rd, len);
  m_ping_timer.reset();
  
} /* RewindLogic::sendPing */


void RewindData::sendServiceData(void)
{
  

} /* RewindData::sendServiceData */


void RewindLogic::sendKeepAlive(void)
{
   RewindData *vd;
   vd->type = htole16(REWIND_TYPE_KEEP_ALIVE);
   vd->flags = htole16(REWIND_FLAG_NONE);
   vd->number = htole32(++sequenceNumber);
   vd->length = htobe16(length);
   
   len += sizeof(struct RewindData);
} /* RewindLogic::sendPong */


void RewindLogic::sendCloseMessage(void)
{
  m_state = DISCONNECTED;
  m_ping_timer.setEnable(false);
} /* RewindLogic::sendCloseMessage */


void RewindLogic::sendConfiguration(void)
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
  cout << "laenge: " << p_msg.length() << endl;
  sendMsg(p_msg);
}


void RewindLogic::handleDataMessage(std::string dmessage)
{
  char const *dmsg = dmessage.c_str();
  int m_sid = atoi(&dmsg[0]); // squence number

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
  //int Rewinddata =

} /* RewindLogic::handleDataMessage */

/*
 * This file has not been truncated
 */

