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
  int sample_rate = 960000;
  cfg.getValue(name, "SAMPLE_RATE", sample_rate);

  cout << "###   HOST      = " << remote_host << endl;
  cout << "###   PORT      = " << tcp_port << endl;
  rtl = new RtlTcp(remote_host, tcp_port);
  rtl->setSampleRate(sample_rate);
  rtl->iqReceived.connect(iqReceived.make_slot());

  uint32_t fq_corr = 0;
  if (cfg.getValue(name, "FQ_CORR", fq_corr))
  {
    cout << "###   FQ_CORR   = " << fq_corr << "ppm\n";
    rtl->setFqCorr(fq_corr);
  }

  uint32_t center_fq = 0;
  if (cfg.getValue(name, "CENTER_FQ", center_fq))
  {
    cout << "###   CENTER_FQ = " << center_fq << "Hz\n";
    auto_tune_enabled = false;
    setCenterFq(center_fq);
  }

  float gain = 0.0f;
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
  cout << "### WbRxRtlTcp::setCenterFq: fq=" << fq << endl;
  rtl->setCenterFq(fq);
  for (Ddrs::iterator it=ddrs.begin(); it!=ddrs.end(); ++it)
  {
    Ddr *ddr = (*it);
    ddr->tunerFqChanged(fq);
  }
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

    // Put all DDR frequencies in a list and sort it
  deque<double> fqs;
  for (Ddrs::iterator it=ddrs.begin(); it!=ddrs.end(); ++it)
  {
    Ddr *ddr = (*it);
    /*
    cout << "### " << ddr->name()
         << ": fq=" << ddr->nbFq()
         << endl;
    */
    fqs.push_back(ddr->nbFq());
  }
  sort(fqs.begin(), fqs.end());

    // Now check that all DDRs fit inside the tuner bandwidth. If not,
    // remove frequencies from the list until it will fit.
  double span = 0.0;
  while ((span = fqs.back() - fqs.front()) > (rtl->sampleRate()-25000))
  {
    cout << "### Span too wide: " << span << endl;
    double front_dist = *(fqs.begin()+1) - fqs.front();
    double back_dist = fqs.back() - *(fqs.rbegin()+1);
    if (front_dist > back_dist)
    {
      fqs.pop_front();
    }
    else
    {
      fqs.pop_back();
    }
  }

    // Now place all channels centralized within the tuner bandwidth
  double center_fq = (fqs.front() + fqs.back()) / 2.0;

    // Check if we have any channel too close to the center of the band.
    // The center of the band almost always contain interference signals.
    // Shift the band to get channels away from the center.
  deque<double>::iterator it = fqs.begin();
  while (it != fqs.end())
  {
    deque<double>::iterator next = it+1;
    double dist = *it - center_fq;
    double next_dist = *next - center_fq;
    if ((next == fqs.end()) || (abs(dist) < abs(next_dist)))
    {
      if (abs(dist) < 12500)
      {
        double max_move = (rtl->sampleRate() - span) / 2;
        if (dist < 0)
        {
          center_fq += min(12500+dist, max_move);
        }
        else
        {
          center_fq -= min(12500-dist, max_move);
        }
      }
    }
    it = next;
  }

  setCenterFq(center_fq);

} /* WbRxRtlTcp::findBestCenterFq */



/*
 * This file has not been truncated
 */

