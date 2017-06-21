/**
@file	 RewindLogic.cpp
@brief   A class to connect a brandmeiseter server with UDP
@author  Artem Prilutskiy / R3ABM & Tobias Blomberg / SM0SVX
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

#include <AsyncAudioDecimator.h>
#include <AsyncAudioInterpolator.h>
#include <AsyncUdpSocket.h>
#include <common.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "RewindLogic.h"
#include "sha256.h"
#include "multirate_filter_coeff.h"
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
  : LogicBase(cfg, name),
    m_state(DISCONNECTED), m_udp_sock(0), m_auth_key("passw0rd"),
    rewind_host(""), rewind_port(54005), m_callsign("N0CALL"),
    m_ping_timer(5000, Timer::TYPE_PERIODIC, false),
    m_reconnect_timer(30000, Timer::TYPE_PERIODIC, false),
    sequenceNumber(0), m_slot1(false), m_slot2(false), subscribed(0)
{
} /* RewindLogic::RewindLogic */


RewindLogic::~RewindLogic(void)
{
  delete m_udp_sock;
  m_ping_timer = 0;
  m_reconnect_timer = 0;
  delete dns;
  delete m_logic_con_in;
  delete m_logic_con_out;
  delete m_logic_enc;
  delete m_dec;
} /* RewindLogic::~RewindLogic */


bool RewindLogic::initialize(void)
{
  if (!cfg().getValue(name(), "HOST", rewind_host))
  {
    cerr << "*** ERROR: " << name() << "/HOST missing in configuration"
         << endl;
    return false;
  }

  cfg().getValue(name(), "PORT", rewind_port);

  if (!cfg().getValue(name(), "CALLSIGN", m_callsign))
  {
    cerr << "*** ERROR: " << name() << "/CALLSIGN missing in configuration"
         << endl;
    return false;
  }
  if (m_callsign.length() > 9)
  {
    cerr << "*** ERROR: CALLSIGN=" << m_callsign << " is to long"
         << endl;
    return false;
  }

  if (!cfg().getValue(name(), "ID", m_id))
  {
    cerr << "*** ERROR: " << name() << "/ID missing in configuration"
         << endl;
    return false;
  }

  if (m_id.length() < 6 || m_id.length() > 7)
  {
    cerr << "*** ERROR: " << name() << "/ID is wrong, must have 6 or 7 digits,"
         << "e.g. ID=2620001" << endl;
    return false;
  }

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

  if (!cfg().getValue(name(), "RECONNECT_INTERVAL", m_rc_interval))
  {
    m_rc_interval = "30000";
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
  if (!cfg().getValue(name(), "TALKGROUPS", m_tg))
  {
    m_tg = "9";
    cout << "+++ INFO: setting default talkgroup 9" << endl;
  }

  // subscripting to one or more Talkgroups
  vector<string> tgs;
  SvxLink::splitStr(tgs, m_tg, ",");
  for(vector<string>::iterator it = tgs.begin(); it != tgs.end(); ++it)
  {
    tglist.push_back(atoi((*it).c_str()));
  }

  m_swid += "linux:SvxLink v";
  m_swid += SVXLINK_VERSION;

  m_ping_timer.setEnable(false);
  m_ping_timer.expired.connect(
     mem_fun(*this, &RewindLogic::pingHandler));

  m_logic_con_in = new AudioSink;
  AudioSource *prev_sc = new AudioSource;
 // AudioSource *prev_sc = m_logic_con_in;
  prev_sc->registerSink(m_logic_con_in,true);

#if INTERNAL_SAMPLE_RATE == 16000
  {
    AudioDecimator *d1 = new AudioDecimator(2, coeff_16_8, coeff_16_8_taps);
    prev_sc->registerSink(d1, true);
    prev_sc = d1;
  }
#endif

  // create the Rewind recoder device, DV3k USB stick or DSD lib
  string m_ambe_handler = "AMBE";

/*  m_logic_enc = Async::AudioEncoder::create(m_ambe_handler);
  if (m_logic_enc == 0)
  {
    cerr << "*** ERROR: Failed to initialize audio encoder" << endl;
    return false;
  }

  m_logic_enc->registerSource(prev_sc);
  m_logic_enc->writeEncodedSamples.connect(
      mem_fun(*this, &RewindLogic::sendEncodedAudio));
  m_logic_enc->flushEncodedSamples.connect(
      mem_fun(*this, &RewindLogic::flushEncodedAudio));
  cout << "Loading Encoder " << m_logic_enc->name() << endl;
*/

  // sending options to audio encoder
  string opt_prefix = m_ambe_handler + "_";
  list<string> names = cfg().listSection(name());
  list<string>::const_iterator nit;
  map<string,string> m_dec_options;
  for (nit=names.begin(); nit!=names.end(); ++nit)
  {
    if ((*nit).find(opt_prefix) == 0)
    {
      string opt_value;
      cfg().getValue(name(), *nit, opt_value);
      string opt_name((*nit).substr(opt_prefix.size()));
      m_dec_options[opt_name]=opt_value;
    }
  }

    // Create audio decoder
  try {
    m_dec = Async::AudioDecoder::create(m_ambe_handler,m_dec_options);
    if (m_dec == 0)
    {
      cerr << "*** ERROR: Failed to initialize audio decoder" << endl;
      return false;
    }
  } catch(const char *e) {
      cerr << e << endl;
      return false;
  }

  m_dec->allEncodedSamplesFlushed.connect(
      mem_fun(*this, &RewindLogic::allEncodedSamplesFlushed));

  cout << "Loading Decoder " << m_dec->name() << endl;
  AudioSource *prev_src = m_dec;

    // Create jitter FIFO if jitter buffer delay > 0
  unsigned jitter_buffer_delay = 0;
  cfg().getValue(name(), "JITTER_BUFFER_DELAY", jitter_buffer_delay);
  if (jitter_buffer_delay > 0)
  {
    AudioFifo *fifo = new Async::AudioFifo(
        2 * jitter_buffer_delay * INTERNAL_SAMPLE_RATE / 1000);
    //new Async::AudioJitterFifo(100 * INTERNAL_SAMPLE_RATE / 1000);
    fifo->setPrebufSamples(jitter_buffer_delay * INTERNAL_SAMPLE_RATE / 1000);
    prev_src->registerSink(fifo, true);
    prev_src = fifo;
  }
  m_logic_con_out = prev_src;

    // upsampling from 8kHz to 16kHz
#if INTERNAL_SAMPLE_RATE == 16000
  AudioInterpolator *up = new AudioInterpolator(2, coeff_16_8, coeff_16_8_taps);
  m_logic_con_out->registerSink(up, true);
  m_logic_con_out = up;
#endif

  if (!LogicBase::initialize())
  {
    return false;
  }

   // connect the master server
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
    dns = new DnsLookup(rewind_host);
    dns->resultsReady.connect(mem_fun(*this, &RewindLogic::dnsResultsReady));
    return;
  }

  cout << name() << ": create UDP socket on port " << rewind_port << endl;

  delete m_udp_sock;
  m_udp_sock = new Async::UdpSocket();
  m_udp_sock->dataReceived.connect(
      mem_fun(*this, &RewindLogic::onDataReceived));

   // session initiated by a simple REWIND_TYPE_KEEP_ALIVE message
  sendKeepAlive();

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

  struct RewindData* rd = reinterpret_cast<RewindData*>(buf);

  switch (rd->type)
  {
    case REWIND_TYPE_REPORT:
      cout << "--- server message: " << rd->data << endl;
      return;

    case REWIND_TYPE_CHALLENGE:
      cout << "*** rewind type Challenge" << endl;
      authenticate(rd->data, m_auth_key);
      subscribed = 0;
      return;

    case REWIND_TYPE_CLOSE:
      cout << "*** Disconnect request received." << endl;
      m_state = DISCONNECTED;
      m_ping_timer.setEnable(false);
      subscribed = 0;
      return;

    case REWIND_TYPE_CANCELLING:
      cout << ">>> Subscription canceled" << endl;
      sendSubscription(tglist);
      subscribed = 0;
      m_state = AUTHENTICATED;
      return;

    case REWIND_TYPE_KEEP_ALIVE:
      if (m_state == WAITING_PASS_ACK)
      {
        cout << "--- Authenticated on " << rewind_host << endl;
        m_state = AUTHENTICATED;
      }
      if (m_state == AUTHENTICATED && ++subscribed < 3)
      {
        sendSubscription(tglist);
      }
      return;

    case REWIND_TYPE_FAILURE_CODE:
      cout << "*** ERROR: sourceCall or destinationCall could not be "
           << "resolved during processing." << endl;
      return;

    case REWIND_TYPE_REMOTE_CONTROL:
      cout << "    type: remote_control " << endl;
      return;

    case REWIND_TYPE_PEER_DATA:
      cout << "*** type: peer data" << endl;
      return;

    case REWIND_TYPE_MEDIA_DATA:
      cout << "*** type: media data" << endl;
      return;

    case REWIND_TYPE_CONFIGURATION:
      cout << "--- Successful sent configuration" << endl;
      m_state = AUTH_CONFIGURATION;
      return;

    case REWIND_TYPE_SUBSCRIPTION:
      cout << "--- Successful subscription, TG's=" << m_tg << endl;
      subscribed = 0;
      sendConfiguration();
      m_state = AUTH_SUBSCRIBED;
      return;

    case REWIND_TYPE_DMR_DATA_BASE:
      cout << "*** type: dmr data base" << endl;
      return;

    case REWIND_TYPE_DMR_EMBEDDED_DATA:
      cout << "*** dmr embedded data" << endl;
      handleDataMessage(rd);
      return;

    case REWIND_TYPE_DMR_AUDIO_FRAME:
      cout << "-- dmr audio frame received" << endl;
      handleAmbeAudiopacket(rd);
      return;

    case REWIND_TYPE_SUPER_HEADER:
      cout << "-- super header received" << endl;
      handleSessionData(rd->data);
      return;

    case REWIND_TYPE_DMR_START_FRAME:
      cout << "--> qso begin" << endl;
      return;

    case REWIND_TYPE_DMR_STOP_FRAME:
      cout << "--> qso end" << endl;
      return;

    default:
      cout << "*** Unknown data received, TYPE=" << rd->type << endl;
  }
} /* RewindLogic::udpDatagramReceived */


void RewindLogic::handleSessionData(uint8_t data[])
{
  struct RewindSuperHeader* shd
       = reinterpret_cast<RewindSuperHeader*>(data);
  cout << "SourceCall=" << shd->sourceCall << " (" << shd->sourceID
       << "), DestCall=" << shd->destinationCall << " dstid=" << shd->destinationID
       << endl;
  srcCall = (char*)shd->sourceCall;
  srcId = shd->sourceID;
} /* RewindLogic::handleSessionData */


void RewindLogic::handleAmbeAudiopacket(struct RewindData* ad)
{
  m_dec->writeEncodedSamples(ad->data, ad->length);
} /* RewindLogic::handleAudioPacket */


void RewindLogic::handleDataMessage(struct RewindData* dm)
{
  uint8_t data[11];
  memcpy(data, dm->data, 10);
  uint8_t sp[11];
  int off;
  std::string ts;

  cout << ">" << dec
  << data[0] << ","
  << data[1] << ","
  << data[2] << ","
  << data[3] << ","
  << data[4] << ","
  << data[5] << ","
  << data[6] << ","
  << data[7] << ","
  << data[8] << ","
  << data[9] << "," << endl;
  printf("%d %d %d %d %d %d %d %d %d %d\n",data[0],data[1],data[2],data[3],
                                            data[4],data[5],data[6],data[7],
                                            data[8],data[9]);

  if (stninfo.find(srcId) == stninfo.end())
  {
    stninfo.insert ( std::pair<int, std::string>(srcId, "                     ") );
  }
  switch (data[0]) {
    case 0x04:
      memcpy(sp,data+3,6);
      ts = (char*)sp;
      (stninfo.find(srcId)->second).replace(0, 6, ts);
      break;
    case 0x05:
      memcpy(sp,data+2,7);
      ts = (char*)sp;
      (stninfo.find(srcId)->second).replace(6, 7, ts);
      break;
    case 0x06:
    case 0x07:
      memcpy(sp,data+2,7);
      ts = (char*)sp;
      off = 7 * (data[0] - 4) - 1;
      (stninfo.find(srcId)->second).replace(off, 7, ts);
      cout << "STATIONINFO: " << stninfo.find(srcId)->second << endl;
      break;
  }
} /* RewindLogic::handleDataMessage */


void RewindLogic::authenticate(uint8_t salt[], const string pass)
{

  size_t size = sizeof(struct RewindData) + SHA256_DIGEST_LENGTH;
  struct RewindData* data = (struct RewindData*)alloca(size);

  memcpy(data->sign, REWIND_PROTOCOL_SIGN, REWIND_SIGN_LENGTH);
  data->type   = htole16(REWIND_TYPE_AUTHENTICATION);
  data->flags  = htole16(REWIND_FLAG_DEFAULT_SET);
  data->length = htole16(SHA256_DIGEST_LENGTH);

  size_t length = pass.size() + sizeof(uint32_t);
  uint8_t* buffer = (uint8_t*)alloca(length);

  memcpy(buffer, salt, sizeof(uint32_t));
  memcpy(buffer + sizeof(uint32_t), pass.c_str(), pass.size());
  mkSHA256(buffer, length, data->data);

  sendMsg(data, size);
  m_state = WAITING_PASS_ACK;
} /* RewindLogic::authPassphrase */


void RewindLogic::sendMsg(struct RewindData* data, size_t len)
{

  if (ip_addr.isEmpty())
  {
    if (!dns)
    {
      dns = new DnsLookup(rewind_host);
      dns->resultsReady.connect(mem_fun(*this, &RewindLogic::dnsResultsReady));
    }
    return;
  }

  if (m_udp_sock == 0)
  {
    cerr << "*** ERROR: No Udp socket available" << endl;
    return;
  }

  data->number = htole32(++sequenceNumber);

  /*cout << "### sending udp packet to " << ip_addr.toString()
       << ":" << rewind_port << ", length=" << len
       << " seq: " << sequenceNumber << " type=" << data->type
       << " inhalt=" << data->number
       << endl;
  */
  m_udp_sock->write(ip_addr, rewind_port, data, len);

} /* RewindLogic::sendUdpMsg */


void RewindLogic::reconnect(Timer *t)
{
  cout << "### Reconnecting to Rewind server\n";
  connect();
  m_reconnect_timer.reset();
} /* RewindLogic::reconnect */


void RewindLogic::allEncodedSamplesFlushed(void)
{
} /* RewindLogic::allEncodedSamplesFlushed */


void RewindLogic::pingHandler(Async::Timer *t)
{
  sendKeepAlive();
} /* RewindLogic::heartbeatHandler */


void RewindLogic::sendKeepAlive(void)
{
  struct RewindData* data = (struct RewindData*)alloca(sizeof(struct RewindData) + BUFFER_SIZE);

  struct RewindVersionData* version = (struct RewindVersionData*)data->data;
  size_t size = m_swid.length();

  version->number = atoi(m_id.c_str());
  version->service = REWIND_SERVICE_SIMPLE_APPLICATION;
  strcpy(version->description, m_swid.c_str());
  size += sizeof(struct RewindVersionData);

  memcpy(data->sign, REWIND_PROTOCOL_SIGN, REWIND_SIGN_LENGTH);
  data->type   = htole16(REWIND_TYPE_KEEP_ALIVE);
  data->flags  = htole16(REWIND_FLAG_NONE);
  data->length = htole16(size);

  size += sizeof(struct RewindData);
  sendMsg(data, size);
  m_ping_timer.reset();

} /* RewindLogic::sendPing */


void RewindLogic::sendServiceData(void)
{
} /* RewindData::sendServiceData */


void RewindLogic::sendCloseMessage(void)
{
  size_t size = sizeof(struct RewindData);
  struct RewindData* data = (struct RewindData*)alloca(size);

  memcpy(data->sign, REWIND_PROTOCOL_SIGN, REWIND_SIGN_LENGTH);
  data->type   = htole16(REWIND_TYPE_CLOSE);
  data->flags  = htole16(REWIND_FLAG_NONE);
  data->length = 0;

  sendMsg(data, size);

  m_state = DISCONNECTED;
  m_ping_timer.setEnable(false);
} /* RewindLogic::sendCloseMessage */


void RewindLogic::sendConfiguration(void)
{
  size_t size = sizeof(struct RewindData) + sizeof(struct RewindConfigurationData);
  struct RewindData* data = (struct RewindData*)alloca(size);

  memcpy(data->sign, REWIND_PROTOCOL_SIGN, REWIND_SIGN_LENGTH);
  data->type   = htole16(REWIND_TYPE_CONFIGURATION);
  data->flags  = htole16(REWIND_FLAG_NONE);         // 0x00
  data->length = htole16(sizeof(struct RewindConfigurationData));

  struct RewindConfigurationData* configuration = (struct RewindConfigurationData*)data->data;
  configuration->options = htole32(REWIND_OPTION_SUPER_HEADER);

  sendMsg(data, size);
} /* RewindLogic::sendConfiguration */


void RewindLogic::sendSubscription(std::list<int> tglist)
{
 size_t size1 = sizeof(struct RewindSubscriptionData);
 size_t size2 = sizeof(struct RewindData) + size1;
 struct RewindData* data = (struct RewindData*)alloca(size2);
 struct RewindSubscriptionData* subscription = (struct RewindSubscriptionData*)data->data;

 memcpy(data->sign, REWIND_PROTOCOL_SIGN, REWIND_SIGN_LENGTH);
 data->type   = htole16(REWIND_TYPE_SUBSCRIPTION);
 data->flags  = htole16(REWIND_FLAG_NONE);
 data->length = htole16(size1);

 subscription->type = htole32(SESSION_TYPE_GROUP_VOICE);

 for (std::list<int>::iterator it = tglist.begin(); it!=tglist.end(); it++)
 {
   subscription->number = htole32(*it);
   sendMsg(data, size2);
 }
} /* RewindLogic::sendConfiguration */


void RewindLogic::cancelSubscription(void)
{
  size_t size = sizeof(struct RewindData);
  struct RewindData* data = (struct RewindData*)alloca(size);

  memcpy(data->sign, REWIND_PROTOCOL_SIGN, REWIND_SIGN_LENGTH);
  data->type   = htole16(REWIND_TYPE_CANCELLING);
  data->flags  = htole16(REWIND_FLAG_NONE);
  data->length = 0;

  sendMsg(data, size);
} /* RewindLogic::cancelSubscription */


void RewindLogic::mkSHA256(uint8_t pass[], int len, uint8_t hash[])
{
  SHA256_CTX context;
  sha256_init(&context);
  sha256_update(&context, pass, len);
  sha256_final(&context, hash);
} /* RewindLogic:mkSha256*/



/*
 * This file has not been truncated
 */

