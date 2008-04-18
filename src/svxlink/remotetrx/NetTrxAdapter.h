/**
@file	 NetTrxAdapter.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2008-04-15

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
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
@brief	A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2008-

A_detailed_class_description

\include NetTrxAdapter_demo.cpp
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
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    bool initialize(void);
    
    Rx *rx(void);
    Tx *tx(void);
    

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
    
};  /* class NetTrxAdapter */


//} /* namespace */

#endif /* NET_TRX_ADAPTER_INCLUDED */



/*
 * This file has not been truncated
 */

