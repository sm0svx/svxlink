/**
@file	 WbRxRtlTcp.cpp
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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdint.h>


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

#include "WbRxRtlTcp.h"
#include "RtlTcp.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;



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

WbRxRtlTcp::InstanceMap WbRxRtlTcp::instances;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

WbRxRtlTcp *WbRxRtlTcp::instance(Async::Config &cfg, const string &name)
{
  InstanceMap::iterator it = instances.find(name);
  if (it != instances.end())
  {
    return (*it).second;
  }
  WbRxRtlTcp *wbrx = new WbRxRtlTcp(cfg, name);
  instances[name] = wbrx;
  return wbrx;
} /* WbRxRtlTcp::instance */


WbRxRtlTcp::WbRxRtlTcp(Async::Config &cfg, const string &name)
{
  cout << "### Initializing WBRX " << name << endl;

  string remote_host = "localhost";
  cfg.getValue(name, "HOST", remote_host);
  int tcp_port = 1234;
  cfg.getValue(name, "PORT", tcp_port);

  cout << "###   HOST      = " << remote_host << endl;
  cout << "###   PORT      = " << tcp_port << endl;
  rtl = new RtlTcp(remote_host, tcp_port);
  rtl->iqReceived.connect(iqReceived.make_slot());

    // Hardcode sampling rate for now since the decimation filters are
    // designed for that specific sampling frequency.
  rtl->setSampleRate(960000);

  uint32_t fq_corr;
  if (cfg.getValue(name, "FQ_CORR", fq_corr))
  {
    cout << "###   FQ_CORR   = " << fq_corr << endl;
    rtl->setFqCorr(fq_corr);
  }

  uint32_t center_fq;
  if (cfg.getValue(name, "CENTER_FQ", center_fq))
  {
    cout << "###   CENTER_FQ = " << fq_corr << endl;
    rtl->setCenterFq(center_fq);
  }

  uint32_t gain;
  if (cfg.getValue(name, "GAIN", gain))
  {
    cout << "###   GAIN      = " << gain << endl;
    rtl->setGainMode(1);
    rtl->setGain(gain);
  }
} /* WbRxRtlTcp::WbRxRtlTcp */


WbRxRtlTcp::~WbRxRtlTcp(void)
{
  delete rtl;
  rtl = 0;
} /* WbRxRtlTcp::~WbRxRtlTcp */


uint32_t WbRxRtlTcp::centerFq(void)
{
  return rtl->centerFq();
} /* WbRxRtlTcp::centerFq */



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



/*
 * This file has not been truncated
 */

