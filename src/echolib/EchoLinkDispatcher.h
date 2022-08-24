/**
@file	 EchoLinkDispatcher.h
@brief   Contains a class for handling incoming/active EchoLink Qso connections
@author  Tobias Blomberg
@date	 2003-03-30

This file contains a class that listens to incoming packets on the two
UDP-ports. The incoming packets are dispatched to the associated connection
object, if it exists. If the connection does not exist, a signal will be emitted
to indicate that an incoming connection is on its way.

\verbatim
EchoLib - A library for EchoLink communication
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

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

/** @example EchoLinkDispatcher_demo.cpp
An example of how to use the EchoLink::Dispatcher class
*/


#ifndef ECHOLINK_DISPATCHER_INCLUDED
#define ECHOLINK_DISPATCHER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

#include <map>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncIpAddress.h>
#include <AsyncUdpSocket.h>


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



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

namespace EchoLink
{

/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

class Qso;


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
@brief	A class for handling incoming connections and dispatch active
      	connections
@author Tobias Blomberg
@date   2003-03-30

This is a class that listens to incoming packets on the two EchoLink UDP-ports.
The incoming packets are dispatched to the associated connection object
(EchoLink::Qso), if it exists. If the connection does not exist, a signal will
be emitted to indicate that an incoming connection is on its way. The Qso
objects will register themselfs automatically with the Dispatcher upon creation.

This class is a Singleton object. That is, there can be only one object. The
constructor is private so the only way to create a Dispatcher object is to use
the static method Dispatcher::instance. It will create a new object if it does
not exist and return the object if it exists. On error, a NULL pointer will be
returned.

Start your application by creating a Dispatcher object. It will then start to
listen to the EchoLink ports for incoming connections. Also, be sure to check
the returned pointer. If it is NULL, something went wrong. If it is not checked,
you will have trouble later when creating Qso objects. So, a typical start in
your application would be something like the code below.

\include EchoLinkDispatcher_demo.cpp
*/
class Dispatcher : public sigc::trackable
{
  public:
    /**
     * @brief 	Set the port base for the two UDP ports
     * @param 	base The port base to use
     *
     * This static function set the port base to use for the two UDP
     * communications ports. This function may only be called when a dispatcher
     * object does not exist. Typically, call this function before calling the
     * Dispatcher::instance function for the first time. If this function is not
     * called at all, the default ports will be used.
     */
    static void setPortBase(int base);

    /**
     * @brief 	Set the bind address for the two UDP ports
     * @param 	ip The IP address to use
     *
     * This static function set the IP-address/interface to bind to for the two
     * UDP communications ports. This may be necessary if the computer is
     * fitted with more than one network interface and only one should be used
     * for EchoLink.  This function may only be called when a dispatcher object
     * does not exist. Typically, call this function before calling the
     * Dispatcher::instance function for the first time. If this function is
     * not called at all, the dispatcher will bind to all available interfaces.
     */
    static void setBindAddr(const Async::IpAddress& ip);
    
    /**
     * @brief 	Get the Singleton instance
     * @return	Returns a dispatcher object. If the object does not already
     *        	exist, a new object will be created. If the object can not be
     *        	created, a NULL pointer is returned.
     *
     * This function should be used to get access to the Singleton dispatcher
     * object. Every time access to the dispatcher object is required, this
     * function should be used to get the pointer. It is illegal to store it in
     * a variable at program startup and then use that variable. It is possible
     * that the dispatcher is deleted at some point in a program. The stored
     * pointer would then be invalid. It is ok to use a local temporary variable
     * to make access more convenient if no calls are made to other functions
     * that access the dispatcher.
     */
    static Dispatcher *instance(void);

    /**
     * @brief  Delete the singleton object if it exists
     */
    static void deleteInstance(void);
    
    /**
     * @brief 	Destructor
     */
    ~Dispatcher(void);
    
    /**
     * @brief 	A signal that is emitted when someone is trying to connect
     * @param 	callsign  The callsign of the connecting station
     * @param 	name  	  The name of the connecting station
     * @param   priv      A private string for passing connection parameters
     *
     * This signal is emitted when a remote station tries to connect. It will be
     * emitted every time a connect datagram is received.
     */
    sigc::signal<void, const Async::IpAddress&, const std::string&,
      	      	  const std::string&, const std::string&> incomingConnection;
    
  protected:
    
  private:
    friend class Qso;
  
    typedef void (Qso::*CtrlInputHandler)(unsigned char *buf, int len);
    typedef void (Qso::*AudioInputHandler)(unsigned char *buf, int len);
    typedef struct
    {
      Qso *      	con;
      CtrlInputHandler	cih;
      AudioInputHandler aih;
    } ConData;
    typedef std::map<Async::IpAddress, ConData> ConMap;
    
    static const int  	DEFAULT_PORT_BASE = 5198;
    
    static int	      	    port_base;
    static Async::IpAddress bind_ip;
    static Dispatcher *     the_instance;
    
    ConMap    	      	    con_map;
    Async::UdpSocket * 	    ctrl_sock;
    Async::UdpSocket * 	    audio_sock;
    
    bool registerConnection(Qso *con, CtrlInputHandler cih,
	AudioInputHandler aih);
    void unregisterConnection(Qso *con);
    
    Dispatcher(void);
    void ctrlDataReceived(const Async::IpAddress& ip, uint16_t port,
                          void *buf, int len);
    void audioDataReceived(const Async::IpAddress& ip, uint16_t port,
                           void *buf, int len);
    void printData(const char *buf, int len);
    
      // These functions are accessed by the Qso class
    bool sendCtrlMsg(const Async::IpAddress& to, const void *buf, int len);
    bool sendAudioMsg(const Async::IpAddress& to, const void *buf, int len);
    
};  /* class Dispatcher */


} /* namespace */

#endif /* ECHOLINK_DISPATCHER_INCLUDED */



/*
 * This file has not been truncated
 */

