/**
@file	 NetTrxAdapter.h
@brief   Make it possible to connect a remote NetTx/NetRx as a Rx/Tx
@author  Tobias Blomberg / SM0SVX
@date	 2008-04-15

\verbatim
RemoteTrx - A remote receiver for the SvxLink server
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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

#ifndef NET_TRX_ADAPTER_INCLUDED
#define NET_TRX_ADAPTER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <map>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>


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

class Uplink;
class TxAdapter;
class RxAdapter;


/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

  

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
@brief	 Make it possible to connect a remote NetTx/NetRx as a Rx/Tx
@author	 Tobias Blomberg / SM0SVX
@date	 2008-04-15

This is a bit of a strange and backwards class. It is used to make it possible
to connect a remote NetTx to a remotetrx Rx and also the other way around,
connect a remote NetRx to a remotetrx Tx.

An example of when this comes handy is when running the following setup:

  - A remotetrx application is running on the local computer
  - One or more remote receivers are linked in through the Internet
  - An RfUplink is used to link to the main system through a transceiver
  - The SvxLink server application is running on the same computer (e.g. to
    provide EchoLink to the main system) as remotetrx and want to use the
    same transceiver as the RfUplink is using. When SvxLink is transmitting
    it will appear to the RemoteTrx as if it is traffic coming in on one of
    its receivers. When the RemoteTrx is transmitting it will appear to SvxLink
    as if it's traffic coming in on its receiver.
*/
class NetTrxAdapter
{
  public:
    static NetTrxAdapter *instance(Async::Config &cfg,
      	      	      	      	   const std::string &net_uplink_name);
    
    /**
     * @brief 	Default constuctor
     */
    NetTrxAdapter(Async::Config &cfg, const std::string &net_uplink_name);
  
    /**
     * @brief 	Destructor
     */
    ~NetTrxAdapter(void);
  
    /**
     * @brief 	Initialize this class
     * @return	Returns \em true on succcess or else \em false
     */
    bool initialize(void);
    
    /**
     * @brief 	Return the associated RX object
     * @return	Returns the associated RX object
     */
    Rx *rx(void) { return reinterpret_cast<Rx *>(rxa1); }
    
    /**
     * @brief 	Return the associated TX object
     * @return	Returns the associated TX object
     */
    Tx *tx(void) { return reinterpret_cast<Tx *>(txa2); }
    

  protected:
    
  private:
    static std::map<std::string, NetTrxAdapter*> net_trx_adapters;
    
    Uplink    	  *ul;
    Async::Config cfg;
    TxAdapter 	  *txa1;
    TxAdapter 	  *txa2;
    RxAdapter 	  *rxa1;
    std::string   net_uplink_name;
    
    NetTrxAdapter(const NetTrxAdapter&);
    NetTrxAdapter& operator=(const NetTrxAdapter&);
    void onTransmittedSignalStrength(RxAdapter *rx, char rx_id, float siglev);
    
};  /* class NetTrxAdapter */


//} /* namespace */

#endif /* NET_TRX_ADAPTER_INCLUDED */



/*
 * This file has not been truncated
 */

