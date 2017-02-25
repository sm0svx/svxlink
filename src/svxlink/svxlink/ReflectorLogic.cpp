/**
@file	 ReflectorLogic.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
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


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpClient.h>
#include <AsyncUdpSocket.h>
#include <AsyncAudioDebugger.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ReflectorLogic.h"
#include "../reflector/ReflectorMsg.h"


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

ReflectorLogic::ReflectorLogic(Async::Config& cfg, const std::string& name)
  : LogicBase(cfg, name), m_msg_type(0), m_udp_sock(0), m_logic_con_in(0),
    m_logic_con_out(0)
{
} /* ReflectorLogic::ReflectorLogic */


ReflectorLogic::~ReflectorLogic(void)
{
  delete m_udp_sock;
  delete m_logic_con_in;
  delete m_logic_con_out;
} /* ReflectorLogic::~ReflectorLogic */


bool ReflectorLogic::initialize(void)
{
  string reflector_host;
  if (!cfg().getValue(name(), "HOST", reflector_host))
  {
    cerr << "*** ERROR: " << name() << "/HOST missing in configuration" << endl;
    return false;
  }

  uint16_t reflector_port = 5300;
  cfg().getValue(name(), "PORT", reflector_port);

  if (!cfg().getValue(name(), "CALLSIGN", m_callsign))
  {
    cerr << "*** ERROR: " << name() << "/CALLSIGN missing in configuration"
         << endl;
    return false;
  }

  if (!cfg().getValue(name(), "PASSWORD", m_reflector_password))
  {
    cerr << "*** ERROR: " << name() << "/PASSWORD missing in configuration"
         << endl;
    return false;
  }

  string audio_codec("OPUS");
  cfg().getValue(name(), "AUDIO_CODEC", audio_codec);

  m_logic_con_in = Async::AudioEncoder::create(audio_codec);
  if (m_logic_con_in == 0)
  {
    cerr << "*** ERROR: Failed to initialize audio encoder" << endl;
    return false;
  }
  m_logic_con_in->writeEncodedSamples.connect(
      mem_fun(*this, &ReflectorLogic::sendEncodedAudio));
  m_logic_con_in->flushEncodedSamples.connect(
      mem_fun(*this, &ReflectorLogic::flushEncodedAudio));
  m_logic_con_out = Async::AudioDecoder::create(audio_codec);
  if (m_logic_con_out == 0)
  {
    cerr << "*** ERROR: Failed to initialize audio decoder" << endl;
    return false;
  }

  if (!LogicBase::initialize())
  {
    return false;
  }

  m_con = new TcpClient(reflector_host, reflector_port);
  m_con->connected.connect(mem_fun(*this, &ReflectorLogic::onConnected));
  m_con->dataReceived.connect(mem_fun(*this, &ReflectorLogic::onDataReceived));
  m_con->connect();

  return true;
} /* ReflectorLogic::initialize */



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

void ReflectorLogic::onConnected(void)
{
  cout << "Connection established to " << m_con->remoteHost() << ":"
       << m_con->remotePort() << endl;
  MsgProtoVer msg;
  sendMsg(msg);
} /* ReflectorLogic::onConnected */


int ReflectorLogic::onDataReceived(TcpConnection *con, void *data, int len)
{
  cout << "### ReflectorLogic::onDataReceived: len=" << len << endl;

  int tot_consumed = 0;
  while (len > 0)
  {
    ReflectorMsg header;
    size_t msg_tot_size = header.packedSize();
    if (static_cast<size_t>(len) < msg_tot_size)
    {
      cout << "### Header data underflow\n";
      return tot_consumed;
    }

    stringstream ss;
    ss.write(reinterpret_cast<const char *>(data), len);

    if (!header.unpack(ss))
    {
      // FIXME: Disconnect
      cout << "*** ERROR: Packing failed for TCP message header\n";
      return tot_consumed;
    }

    msg_tot_size += header.size();
    if (static_cast<size_t>(len) < msg_tot_size)
    {
      cout << "### Payload data underflow\n";
      return tot_consumed;
    }

    switch (header.type())
    {
      case MsgError::TYPE:
        handleMsgError(ss);
        break;
      case MsgAuthChallenge::TYPE:
        handleMsgAuthChallenge(ss);
        break;
      case MsgAuthOk::TYPE:
        handleMsgAuthOk();
        break;
      case MsgServerInfo::TYPE:
        handleMsgServerInfo(ss);
        break;
      default:
        cerr << "*** WARNING: Unknown protocol message received: msg_type="
             << header.type() << endl;
        break;
    }

    data += msg_tot_size;
    len -= msg_tot_size;
    tot_consumed += msg_tot_size;
  }

  return tot_consumed;


#if 0
  m_unp.reserve_buffer(len);
  memcpy(m_unp.buffer(), data, len);
  m_unp.buffer_consumed(len);
  msgpack::unpacked result;
#if MSGPACK_VERSION_MAJOR < 1
  while (m_unp.next(&result))
#else
  while (m_unp.next(result))
#endif
  {
    msgpack::object obj(result.get());
    //cout << obj << endl;
    try
    {
      if (m_msg_type == 0)
      {
        unsigned msg_type = 0;
#if MSGPACK_VERSION_MAJOR < 1
        obj.convert(&msg_type);
#else
        obj.convert(msg_type);
#endif
        switch (msg_type)
        {
          case MsgAuthOk::TYPE:
            handleMsgAuthOk();
            break;
          default:
            m_msg_type = msg_type;
            break;
        }
      }
      else
      {
        switch (m_msg_type)
        {
          case MsgError::TYPE:
            handleMsgError(obj);
            break;
          case MsgAuthChallenge::TYPE:
            handleMsgAuthChallenge(obj);
            break;
          case MsgServerInfo::TYPE:
            handleMsgServerInfo(obj);
            break;
          default:
            cerr << "*** WARNING: Unknown protocol message received: "
                    "message type="
                 << m_msg_type << endl;
            // FIXME: Disconnect?
            break;
        }
        m_msg_type = 0;
      }
    }
    catch (msgpack::unpack_error)
    {
      cerr << "*** WARNING: Failed to unpack incoming message"
           << endl;
      m_msg_type = 0;
      // FIXME: Disconnect!
      //con->disconnect();
    }
    catch (msgpack::type_error)
    {
      cerr << "*** WARNING: The incoming message type have the wrong object type"
           << endl;
      m_msg_type = 0;
      // FIXME: Disconnect!
      //con->disconnect();
    }
  }
#endif
  return len;
} /* ReflectorLogic::onDataReceived */


void ReflectorLogic::handleMsgError(std::istream& is)
{
  MsgError msg;
  if (!msg.unpack(is))
  {
    // FIXME: Disconnect
    cerr << "*** ERROR: Could not unpack MsgAuthChallenge\n";
    return;
  }
  cout << "### " << name() << ": MsgError(\"" << msg.message() << "\")" << endl;
  // FIXME: Handle reconnection
  m_con->disconnect();
} /* ReflectorLogic::handleMsgError */


void ReflectorLogic::handleMsgAuthChallenge(std::istream& is)
{
  MsgAuthChallenge msg;
  if (!msg.unpack(is))
  {
    // FIXME: Disconnect
    cerr << "*** ERROR: Could not unpack MsgAuthChallenge\n";
    return;
  }
  stringstream ss;
  ss << dec << setw(2) << setfill('0');
  for (int i=0; i<MsgAuthChallenge::CHALLENGE_LEN; ++i)
  {
    ss << (int)msg.challenge()[i] << " ";
  }
  cout << "### " << name() << ": MsgAuthChallenge(" << ss.str() << ")" << endl;

  MsgAuthResponse response_msg(m_callsign, m_reflector_password,
                               msg.challenge());
  sendMsg(response_msg);
} /* ReflectorLogic::handleMsgAuthChallenge */


void ReflectorLogic::handleMsgAuthOk(void)
{
  cout << "### " << name() << ": MsgAuthOk()" << endl;
} /* ReflectorLogic::handleMsgAuthChallenge */


void ReflectorLogic::handleMsgServerInfo(std::istream& is)
{
  MsgServerInfo msg;
  if (!msg.unpack(is))
  {
    // FIXME: Disconnect
    cerr << "*** ERROR: Could not unpack MsgAuthChallenge\n";
    return;
  }
  cout << "### " << name() << ": MsgServerInfo(" << msg.clientId() << ")"
       << endl;
  m_client_id = msg.clientId();

  m_udp_sock = new UdpSocket;
  m_udp_sock->dataReceived.connect(
      mem_fun(*this, &ReflectorLogic::udpDatagramReceived));

  sendUdpMsg(MsgUdpHeartbeat());

} /* ReflectorLogic::handleMsgAuthChallenge */


void ReflectorLogic::sendMsg(ReflectorMsg& msg)
{
  ostringstream ss;
  msg.setSize(msg.packedSize());
  if (!msg.ReflectorMsg::pack(ss) || !msg.pack(ss))
  {
    // FIXME: Better error handling
    cerr << "*** ERROR: Failed to pack reflector TCP message\n";
    return;
  }
  m_con->write(ss.str().data(), ss.str().size());

#if 0
  msgpack::sbuffer msg_buf;
  msgpack::pack(msg_buf, msg.type());
  msgpack::pack(msg_buf, msg);
  m_con->write(msg_buf.data(), msg_buf.size());
#endif
} /* ReflectorLogic::sendMsg */


void ReflectorLogic::sendEncodedAudio(const void *buf, int count)
{
  //cout << "### " << name() << ": ReflectorLogic::sendEncodedAudio: count="
  //     << count << endl;
  sendUdpMsg(MsgAudio(buf, count));
} /* ReflectorLogic::sendEncodedAudio */


void ReflectorLogic::flushEncodedAudio(void)
{
  //cout << "### " << name() << ": ReflectorLogic::flushEncodedAudio" << endl;
  sendUdpMsg(MsgAudio());
  m_logic_con_in->allEncodedSamplesFlushed();
} /* ReflectorLogic::sendEncodedAudio */


void ReflectorLogic::udpDatagramReceived(const IpAddress& addr, uint16_t port,
                                         void *buf, int count)
{
  cout << "### " << name() << ": ReflectorLogic::udpDatagramReceived: addr="
       << addr << " port=" << port << " count=" << count;
  std::cout << std::endl;

  stringstream ss;
  ss.write(reinterpret_cast<const char *>(buf), count);

  ReflectorUdpMsg header;
  if (!header.unpack(ss))
  {
    // FIXME: Disconnect
    cout << "*** ERROR: Unpacking failed for UDP message header\n";
    return;
  }

  cout << "###   msg_type=" << header.type()
       << " client_id=" << header.clientId() << std::endl;

  // FIXME: Check remote IP and port number. Maybe also client ID?

  switch (header.type())
  {
    case MsgHeartbeat::TYPE:
      cout << "MsgUdpHeartbeat()" << endl;
      // FIXME: Handle heartbeat
      break;
    case MsgAudio::TYPE:
    {
      MsgAudio msg;
      msg.unpack(ss);
      if (msg.audioData().empty())
      {
        m_logic_con_out->flushEncodedSamples();
      }
      else
      {
        m_logic_con_out->writeEncodedSamples(
            &msg.audioData().front(), msg.audioData().size());
      }
      break;
    }
    default:
      cerr << "*** WARNING: Unknown UDP protocol message received: msg_type="
           << header.type() << endl;
      // FIXME: Disconnect or ignore?
      break;
  }



#if 0
  try
  {
    msgpack::unpacked result;
    size_t offset = 0;
#if 0
    unpack(result, reinterpret_cast<const char *>(buf), count, offset);
    msgpack::object client_id_obj(result.get());
    uint32_t client_id = 0;
    client_id_obj.convert(client_id);
    cout << " client_id=" << client_id_obj;
    ReflectorClientMap::iterator it = client_map.find(client_id);
    if (it == client_map.end())
    {
      cerr << "*** WARNING: Incoming UDP packet has invalid client id" << endl;
      return;
    }
    ReflectorClient *client = (*it).second;
    if (addr != client->remoteHost())
    {
      cerr << "*** WARNING: Incoming UDP packet has the wrong source ip" << endl;
      return;
    }
    if (client->remoteUdpPort() == 0)
    {
      client->setRemoteUdpPort(port);
    }
    else if (port != client->remoteUdpPort())
    {
      cerr << "*** WARNING: Incoming UDP packet has the wrong source UDP "
              "port number" << endl;
      return;
    }
#endif

#if MSGPACK_VERSION_MAJOR < 1
    unpack(&result, reinterpret_cast<const char *>(buf), count, &offset);
#else
    unpack(result, reinterpret_cast<const char *>(buf), count, offset);
#endif
    msgpack::object msg_type_obj(result.get());
    //cout << " msg_type_obj=" << msg_type_obj;

#if MSGPACK_VERSION_MAJOR < 1
    unpack(&result, reinterpret_cast<const char *>(buf), count, &offset);
#else
    unpack(result, reinterpret_cast<const char *>(buf), count, offset);
#endif
    msgpack::object msg_data_obj(result.get());
    //cout << " msg_data_obj=" << msg_data_obj;
    //cout << endl;

    unsigned msg_type = 0;
#if MSGPACK_VERSION_MAJOR < 1
    msg_type_obj.convert(&msg_type);
#else
    msg_type_obj.convert(msg_type);
#endif
    switch (msg_type)
    {
      case MsgAudio::TYPE:
      {
        MsgAudio msg(msg_data_obj);
        if (msg.audioData().empty())
        {
          m_logic_con_out->flushEncodedSamples();
        }
        else
        {
          m_logic_con_out->writeEncodedSamples(
              &msg.audioData().front(), msg.audioData().size());
        }
        break;
      }
      default:
        cerr << "*** WARNING: Unknown UDP protocol message received: msg_type="
             << msg_type << endl;
        // FIXME: Disconnect client or ignore?
        break;
    }
  }
#if MSGPACK_VERSION_MAJOR >= 1
  catch (msgpack::insufficient_bytes)
  {
    cerr << "*** WARNING: The incoming UDP message is too short" << endl;
    // FIXME: Disconnect client or ignore?
    //client->disconnect("Protocol error");
  }
#endif
  catch (msgpack::unpack_error)
  {
    cerr << "*** WARNING: Failed to unpack incoming message"
         << endl;
    // FIXME: Disconnect!
    //con->disconnect();
  }
  catch (msgpack::type_error)
  {
    cerr << "*** WARNING: The incoming UDP message type have the wrong "
            "object type" << endl;
    // FIXME: Disconnect client or ignore?
    //client->disconnect("Protocol error");
  }
#endif
} /* ReflectorLogic::udpDatagramReceived */


void ReflectorLogic::sendUdpMsg(const ReflectorUdpMsg& msg)
{
  if (m_udp_sock == 0)
  {
    return;
  }

  ReflectorUdpMsg header(msg.type(), m_client_id);
  ostringstream ss;
  if (!header.pack(ss) || !msg.pack(ss))
  {
    // FIXME: Better error handling
    cerr << "*** ERROR: Failed to pack reflector TCP message\n";
    return;
  }
  m_udp_sock->write(m_con->remoteHost(), m_con->remotePort(),
                    ss.str().data(), ss.str().size());

#if 0
  msgpack::sbuffer msg_buf;
  msgpack::pack(msg_buf, m_client_id);
  msgpack::pack(msg_buf, msg.type());
  if (msg.haveData())
  {
    msgpack::pack(msg_buf, msg);
  }
  m_udp_sock->write(m_con->remoteHost(), m_con->remotePort(),
                    msg_buf.data(), msg_buf.size());
#endif
} /* ReflectorLogic::sendUdpMsg */



/*
 * This file has not been truncated
 */

