/**
@file	 AsyncTcpServer.cpp
@brief   A class for creating a TCP server
@author  Tobias Blomberg
@date	 2003-12-07

This class is used to create a TCP server that listens to a specified TCP-port.

\verbatim
<A brief description of the program or library this file belongs to>
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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFdWatch.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncTcpServer.h"



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


/*
 *------------------------------------------------------------------------
 * Method:    TcpServer::TcpServer
 * Purpose:   Consturctor
 * Input:     
 * Output:    None
 * Author:    Tobias Blomberg
 * Created:   2003-12-07
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
TcpServer::TcpServer(const string& port_str)
  : sock(-1), rd_watch(0)
{
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    cleanup();
    return;
  }
  
  /* Force close on exec */
  fcntl(sock, F_SETFD, 1);
  
    /* Reuse address if server crashes */
  const int on = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
  
    /* Send small packets at once. */
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));
  
  char *endptr = 0;
  struct servent *se;
  unsigned short port = strtol(port_str.c_str(), &endptr, 10);
  if (*endptr != '\0')
  {
    if ((se = getservbyname(port_str.c_str(), "tcp")) != NULL)
    {
      port = ntohs(se->s_port);
    }
    else
    {
      cerr << "Could not find service " << port_str << endl;
      cleanup();
      return;
    }
  }
  
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
  {
    perror("bind");
    cleanup();
    return;
  }

  if (listen(sock, 5) != 0)
  {
    perror("listen");
    cleanup();
    return;
  }

  rd_watch = new FdWatch(sock, FdWatch::FD_WATCH_RD);
  rd_watch->activity.connect(slot(this, &TcpServer::incomingConnection));
    
} /* TcpServer::TcpServer */


/*
 *------------------------------------------------------------------------
 * Method:    TcpServer::~TcpServer
 * Purpose:   Destructor
 * Input:     
 * Output:    None
 * Author:    Tobias Blomberg
 * Created:   2003-12-07
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
TcpServer::~TcpServer(void)
{
  cleanup();
} /* TcpServer::~TcpServer */



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
 * Method:    TcpServer::cleanup
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    Tobias Blomberg
 * Created:   2003-12-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void TcpServer::cleanup(void)
{
  delete rd_watch;
  rd_watch = 0;
  
  if (sock != -1)
  {
    close(sock);
    sock = -1;
  }
} /* TcpServer::cleanup */


/*
 *----------------------------------------------------------------------------
 * Method:    TcpServer::incomingConnection
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    Tobias Blomberg
 * Created:   2003-12-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void TcpServer::incomingConnection(FdWatch *watch)
{
  int client_sock;
  struct sockaddr_in client;
  socklen_t addrlen = sizeof(client);
  client_sock = accept(sock, (struct sockaddr *)&client, &addrlen);
  if (client_sock == -1)
  {
    perror("accept");
    return;
  }
  
  /* Force close on exec */
  fcntl(client_sock, F_SETFD, 1);
  
    /* Write must not block! */
  fcntl(client_sock, F_SETFL, O_NONBLOCK);
  
    /* Send small packets at once. */
  const int on = 1;
  setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));
  
  TcpConnection *con = new TcpConnection(client_sock,
      	  IpAddress(client.sin_addr), ntohs(client.sin_port));
  clientConnected(con);
  
} /* TcpServer::incomingConnection */






/*
 * This file has not been truncated
 */

