/**
@file	 EchoLinkDirectoryCon.cpp
@brief   EchoLink directory server connection
@author  Tobias Blomberg / SM0SVX
@date	 2013-04-27

\verbatim
EchoLib - A library for EchoLink communication
Copyright (C) 2003-2022 Tobias Blomberg / SM0SVX

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

DirectoryCon::DirectoryCon(const vector<string> &servers,
                           const IpAddress &bind_ip)
  : servers(servers), client(0), last_disconnect_reason(0), is_ready(false)
{
  Proxy *proxy = Proxy::instance();
  if (proxy != 0)
  {
    proxy->proxyReady.connect(mem_fun(*this, &DirectoryCon::proxyReady));
    proxy->tcpConnected.connect(connected.make_slot());
    proxy->tcpDisconnected.connect(disconnected.make_slot());
    proxy->tcpDataReceived.connect(dataReceived.make_slot());
  }
  else
  {
    client = new TcpClient<>;
    client->setBindIp(bind_ip);
    client->connected.connect(connected.make_slot());
    client->disconnected.connect(mem_fun(*this, &DirectoryCon::onDisconnected));
    client->dataReceived.connect(mem_fun(*this, &DirectoryCon::onDataReceived));
    is_ready = true;
    ready(true);
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
    // Remove all pending DNS queries, if any
  vector<DnsLookup*>::iterator it;
  for (it = dns_lookups.begin(); it != dns_lookups.end(); ++it)
  {
    delete *it;
  }
  dns_lookups.clear();

  Proxy *proxy = Proxy::instance();
  if (proxy == 0)
  {
    bool was_idle = client->isIdle();
    client->disconnect();
    if (!was_idle)
    {
      last_disconnect_reason = TcpClient<>::DR_ORDERED_DISCONNECT;
      disconnected();
    }
  }
  else
  {
    last_disconnect_reason = TcpClient<>::DR_ORDERED_DISCONNECT;
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
      errno = EIO;
      return -1;
    }
  }
} /* DirectoryCon::write */


bool DirectoryCon::isIdle(void) const
{
  Proxy *proxy = Proxy::instance();
  if (proxy == 0)
  {
    return is_ready && client->isIdle();
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
    if ((*it)->isPending())
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

  if (addresses.empty())
  {
    cerr << "*** ERROR: No IP addresses were returned for the EchoLink "
            "directory server DNS query\n";
    last_disconnect_reason = TcpConnection::DR_HOST_NOT_FOUND;
    disconnected();
    return;
  }

  current_server = addresses.begin();
  doConnect();
} /* DirectoryCon::onDnsLookupResultsReady */


void DirectoryCon::doConnect(void)
{
  Proxy *proxy = Proxy::instance();
  if (proxy == 0)
  {
    //cout << "### Connecting to " << *current_server << endl;
    client->connect(*current_server, DIRECTORY_SERVER_PORT);
  }
  else
  {
    last_disconnect_reason = TcpClient<>::DR_REMOTE_DISCONNECTED;
    if (!proxy->tcpOpen(*current_server))
    {
      cerr << "*** ERROR: Could not connect to EchoLink directory server "
              "via proxy\n";
      last_disconnect_reason = TcpClient<>::DR_SYSTEM_ERROR;
      errno = ECONNREFUSED;
      disconnected();
    }
  }
} /* DirectoryCon::doConnect */


void DirectoryCon::onDisconnected(TcpConnection *con,
                                  TcpClient<>::DisconnectReason reason)
{
  if (++current_server == addresses.end())
  {
    addresses.clear();
  }
  last_disconnect_reason = static_cast<int>(reason);
  disconnected();
} /* DirectoryCon::onDisconnected */


int DirectoryCon::onDataReceived(TcpConnection *con, void *data, int len)
{
  return dataReceived(data, static_cast<unsigned>(len));
} /* DirectoryCon::onDataReceived */


void DirectoryCon::proxyReady(bool is_ready)
{
  this->is_ready = is_ready;
  ready(is_ready);
} /* DirectoryCon::proxyReady */



/*
 * This file has not been truncated
 */

