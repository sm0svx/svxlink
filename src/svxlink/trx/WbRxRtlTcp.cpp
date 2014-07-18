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
#include <cassert>
#include <limits>
#include <algorithm>
#include <deque>


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
#include "Ddr.h"



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
  : auto_tune_enabled(true), m_name(name)
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
    cout << "###   FQ_CORR   = " << fq_corr << "ppm\n";
    rtl->setFqCorr(fq_corr);
  }

  uint32_t center_fq;
  if (cfg.getValue(name, "CENTER_FQ", center_fq))
  {
    cout << "###   CENTER_FQ = " << center_fq << "Hz\n";
    auto_tune_enabled = false;
    setCenterFq(center_fq);
  }

  float gain;
  if (cfg.getValue(name, "GAIN", gain))
  {
    cout << "###   GAIN      = " << gain << "dB\n";
    rtl->setGainMode(1);
    int32_t int_gain = static_cast<int32_t>(10.0 * gain);
    rtl->setGain(int_gain);
  }

  bool peak_meter = false;
  cfg.getValue(name, "PEAK_METER", peak_meter);
  rtl->enableDistPrint(peak_meter);
} /* WbRxRtlTcp::WbRxRtlTcp */


WbRxRtlTcp::~WbRxRtlTcp(void)
{
  delete rtl;
  rtl = 0;
} /* WbRxRtlTcp::~WbRxRtlTcp */


void WbRxRtlTcp::setCenterFq(uint32_t fq)
{
  rtl->setCenterFq(fq);
  centerFqChanged(fq);
} /* WbRxRtlTcp::setCenterFq */


uint32_t WbRxRtlTcp::centerFq(void)
{
  return rtl->centerFq();
} /* WbRxRtlTcp::centerFq */


uint32_t WbRxRtlTcp::sampleRate(void) const
{
  return rtl->sampleRate();
} /* WbRxRtlTcp::sampleRate */


void WbRxRtlTcp::registerDdr(Ddr *ddr)
{
  Ddrs::iterator it = ddrs.find(ddr);
  assert(it == ddrs.end());
  ddrs.insert(ddr);
  if (auto_tune_enabled)
  {
    findBestCenterFq();
  }
} /* WbRxRtlTcp::registerDdr */


void WbRxRtlTcp::unregisterDdr(Ddr *ddr)
{
  Ddrs::iterator it = ddrs.find(ddr);
  assert(it != ddrs.end());
  ddrs.erase(it);
  if (auto_tune_enabled)
  {
    findBestCenterFq();
  }

    // Delete myself if this was the last DDR
  if (ddrs.empty())
  {
    delete this;
  }
} /* WbRxRtlTcp::unregisterDdr */



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

void WbRxRtlTcp::findBestCenterFq(void)
{
  if (ddrs.empty())
  {
    return;
  }

  deque<double> fqs;
  for (Ddrs::iterator it=ddrs.begin(); it!=ddrs.end(); ++it)
  {
    Ddr *ddr = (*it);
    cout << "### " << ddr->name()
         << ": fq=" << ddr->nbFq()
         << endl;
    fqs.push_back(ddr->nbFq());
  }
  sort(fqs.begin(), fqs.end());
  double span;
  while ((span = fqs.back() - fqs.front()) > rtl->sampleRate())
  {
    cout << "### Span too wide: " << span << endl;
    deque<double>::iterator it =
      upper_bound(fqs.begin(), fqs.end(), span / 2.0);
    size_t lo_cnt = it - fqs.begin();
    size_t hi_cnt = fqs.size() - lo_cnt;
    if (hi_cnt < lo_cnt)
    {
      fqs.pop_back();
    }
    else
    {
      fqs.pop_front();
    }
  }

  double center_fq = (fqs.front() + fqs.back()) / 2.0;

  deque<double>::iterator hi_bound =
    upper_bound(fqs.begin(), fqs.end(), center_fq + 12500);
  deque<double>::iterator lo_bound =
    lower_bound(fqs.begin(), fqs.end(), center_fq - 12500);
  if (lo_bound != fqs.end())
  {
    cout << "### lo_bound=" << *lo_bound << endl;
  }
  if (hi_bound != fqs.end())
  {
    cout << "### hi_bound=" << *hi_bound << endl;
  }
  assert(lo_bound != fqs.end());
  if (lo_bound < hi_bound)
  {
    center_fq += 25000;
  }

  cout << "### New center_fq=" << center_fq << endl;
  setCenterFq(center_fq);

} /* WbRxRtlTcp::findBestCenterFq */


/*
 * This file has not been truncated
 */

