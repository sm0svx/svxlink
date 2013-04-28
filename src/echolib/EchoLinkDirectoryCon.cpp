/**
@file	 EchoLinkDirectoryCon.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2010-

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003-2010 Tobias Blomberg / SM0SVX

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

#include <errno.h>


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

#include "EchoLinkProxy.h"
#include "EchoLinkDirectoryCon.h"



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

DirectoryCon::DirectoryCon(const vector<string> &servers)
  : servers(servers), client(0), last_disconnect_reason(0), is_ready(false)
{
  Proxy *proxy = Proxy::instance();
  if (proxy != 0)
  {
    /*
    proxy->tcpStatusReceived.connect(
        mem_fun(*this, &DirectoryCon::proxyTcpStatusReceived));
    proxy->tcpCloseReceived.connect(
        mem_fun(*this, &DirectoryCon::proxyTcpCloseReceived));
    */
    proxy->proxyReady.connect(mem_fun(*this, &DirectoryCon::proxyReady));
    proxy->tcpConnected.connect(connected.make_slot());
    proxy->tcpDisconnected.connect(disconnected.make_slot());
    proxy->tcpDataReceived.connect(dataReceived.make_slot());
  }
  else
  {
    client = new TcpClient;
    client->connected.connect(connected.make_slot());
    client->disconnected.connect(mem_fun(*this, &DirectoryCon::onDisconnected));
    client->dataReceived.connect(mem_fun(*this, &DirectoryCon::onDataReceived));
    is_ready = true;
    ready(is_ready);
  }
} /* DirectoryCon::DirectoryCon */


DirectoryCon::~DirectoryCon(void)
{
  disconnect();
  delete client;
  client = 0;
} /* DirectoryCon::~DirectoryCon */


void DirectoryCon::connect(void)
{
  if (addresses.empty())
  {
    doDnsLookup();
  }
  else
  {
    doConnect();
  }
} /* DirectoryCon::connect */


void DirectoryCon::disconnect(void)
{
  Proxy *proxy = Proxy::instance();
  if (proxy == 0)
  {
    bool was_connected = client->isConnected();
    client->disconnect();
    vector<DnsLookup*>::iterator it;
    for (it = dns_lookups.begin(); it != dns_lookups.end(); ++it)
    {
      delete *it;
    }
    dns_lookups.clear();
    if (was_connected)
    {
      last_disconnect_reason = TcpClient::DR_ORDERED_DISCONNECT;
      disconnected();
    }
  }
  else
  {
    last_disconnect_reason = TcpClient::DR_ORDERED_DISCONNECT;
    if (!proxy->tcpClose())
    {
      cerr << "*** ERROR: EchoLink proxy TCP close failed\n";
    }
  }
} /* DirectoryCon::disconnect */


int DirectoryCon::write(const void *data, unsigned len)
{
  Proxy *proxy = Proxy::instance();
  if (proxy == 0)
  {
    return client->write(data, len);
  }
  else
  {
    if (proxy->tcpData(data, len))
    {
      return len;
    }
    else
    {
      return -1;
    }
  }
} /* DirectoryCon::write */


bool DirectoryCon::isIdle(void) const
{
  Proxy *proxy = Proxy::instance();
  if (proxy == 0)
  {
    return is_ready && !client->isConnected();
  }
  else
  {
    return is_ready && (proxy->tcpState() == Proxy::TCP_STATE_DISCONNECTED);
  }
} /* DirectoryCon::isIdle */



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

void DirectoryCon::doDnsLookup(void)
{
  vector<string>::const_iterator it;
  for (it = servers.begin(); it != servers.end(); ++it)
  {
    DnsLookup *dns_lookup = new DnsLookup(*it);
    dns_lookup->resultsReady.connect(
        mem_fun(*this, &DirectoryCon::onDnsLookupResultsReady));
    dns_lookups.push_back(dns_lookup);
  }
} /* DirectoryCon::doDnsLookup */


void DirectoryCon::onDnsLookupResultsReady(DnsLookup &dns)
{
    // Check if all lookups are done and also find out how many
    // IP addresses we have looked up totally
  unsigned num_ips = 0;
  vector<DnsLookup*>::iterator it;
  for (it = dns_lookups.begin(); it != dns_lookups.end(); ++it)
  {
    if (!(*it)->resultsAreReady())
    {
      return;
    }
    num_ips += (*it)->addresses().size();
  }

  addresses.clear();
  addresses.reserve(num_ips);
  
    // Populate the addresses vector with looked up IP addresses
  for (it = dns_lookups.begin(); it != dns_lookups.end(); ++it)
  {
    vector<IpAddress> addrs = (*it)->addresses();
    addresses.insert(addresses.end(), addrs.begin(), addrs.end());
    delete *it;
  }
  dns_lookups.clear();

  cout << "DNS lookup done: ";
  vector<IpAddress>::const_iterator ait;
  for (ait = addresses.begin(); ait != addresses.end(); ++ait)
  {
    cout << *ait << " ";
  }
  cout << endl;

  if (addresses.empty())
  {
    cerr << "*** ERROR: No IP addresses were returned for the EchoLink DNS "
            "query\n";
    return;
  }

  current_server = addresses.begin();
  doConnect();
} /* DirectoryCon::onDnsLookupResultsReady */


void DirectoryCon::doConnect(void)
{
  /*
  if (current_server == addresses.end())
  {
    current_server = addresses.begin();
  }
  */

  Proxy *proxy = Proxy::instance();
  if (proxy == 0)
  {
    cout << "Connecting to " << *current_server << endl;
    client->connect(*current_server, DIRECTORY_SERVER_PORT);
  }
  else
  {
    last_disconnect_reason = TcpClient::DR_REMOTE_DISCONNECTED;
    if (!proxy->tcpOpen(*current_server))
    {
      cerr << "*** ERROR: Could not connect to EchoLink directory server "
              "via proxy\n";
      last_disconnect_reason = TcpClient::DR_SYSTEM_ERROR;
      errno = ECONNREFUSED;
      disconnected();
    }
  }
} /* DirectoryCon::doConnect */


void DirectoryCon::onDisconnected(TcpConnection *con,
                                  TcpClient::DisconnectReason reason)
{
  last_disconnect_reason = static_cast<int>(reason);
  disconnected();
} /* DirectoryCon::onDisconnected */


int DirectoryCon::onDataReceived(TcpConnection *con, void *data, int len)
{
  return dataReceived(data, static_cast<unsigned>(len));
} /* DirectoryCon::onDataReceived */


#if 0
void DirectoryCon::proxyTcpStatusReceived(uint32_t status)
{
  cout << "Proxy TCP status: " << status << endl;
  if (status == 0)
  {
    connected();
  }
  else
  {
    // FIXME: Set disconnect reason
    disconnected();
  }
} /* DirectoryCon::proxyTcpStatusReceived */
#endif


void DirectoryCon::proxyReady(bool is_ready)
{
  this->is_ready = is_ready;
  ready(is_ready);
} /* DirectoryCon::proxyReady */


#if 0
int DirectoryCon::proxyTcpDataReceived(void *data, unsigned len)
{
  return dataReceived(data, len);
} /* DirectoryCon::proxyTcpDataReceived */
#endif


#if 0
void DirectoryCon::proxyTcpCloseReceived(void)
{
  //FIXME: Set disconnect reason
  disconnected();
} /* DirectoryCon::proxyTcpCloseReceived */
#endif


/*
 * This file has not been truncated
 */

