/**
@file	 WbRxRtlTcp.h
@brief   A WBRX using RTL2832U based DVB-T tuners
@author  Tobias Blomberg / SM0SVX
@date	 2014-07-16

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
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

/** @example WbRxRtlTcp_demo.cpp
An example of how to use the WbRxRtlTcp class
*/


#ifndef WBRX_RTL_TCP_INCLUDED
#define WBRX_RTL_TCP_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

#include <map>
#include <string>
#include <vector>
#include <complex>


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



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Config;
};
class RtlTcp;


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
@date   2014-07-16

A_detailed_class_description

\include WbRxRtlTcp_demo.cpp
*/
class WbRxRtlTcp
{
  public:
    typedef std::complex<float> Sample;

    static WbRxRtlTcp *instance(Async::Config &cfg, const std::string &name);

    /**
     * @brief 	Default constructor
     */
    WbRxRtlTcp(Async::Config &cfg, const std::string &name);
  
    /**
     * @brief 	Destructor
     */
    ~WbRxRtlTcp(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    uint32_t centerFq(void);

    sigc::signal<void, std::vector<Sample> > iqReceived;
    
  protected:
    
  private:
    typedef std::map<std::string, WbRxRtlTcp*> InstanceMap;

    static InstanceMap instances;

    RtlTcp *rtl;

    WbRxRtlTcp(const WbRxRtlTcp&);
    WbRxRtlTcp& operator=(const WbRxRtlTcp&);
    
};  /* class WbRxRtlTcp */


//} /* namespace */

#endif /* WBRX_RTL_TCP_INCLUDED */



/*
 * This file has not been truncated
 */
