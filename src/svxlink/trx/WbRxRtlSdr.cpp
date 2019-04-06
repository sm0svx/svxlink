/**
@file	 WbRxRtlSdr.cpp
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
#include <iterator>


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

#include "WbRxRtlSdr.h"
#include "RtlTcp.h"
#ifdef HAS_RTLSDR_SUPPORT
#include "RtlUsb.h"
#endif
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

WbRxRtlSdr::InstanceMap WbRxRtlSdr::instances;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

WbRxRtlSdr *WbRxRtlSdr::instance(Async::Config &cfg, const string &name)
{
  InstanceMap::iterator it = instances.find(name);
  if (it != instances.end())
  {
    return (*it).second;
  }
  WbRxRtlSdr *wbrx = new WbRxRtlSdr(cfg, name);
  instances[name] = wbrx;
  return wbrx;
} /* WbRxRtlSdr::instance */


WbRxRtlSdr::WbRxRtlSdr(Async::Config &cfg, const string &name)
  : auto_tune_enabled(true), m_name(name), xvrtr_offset(0)
{
  //cout << "### Initializing WBRX " << name << endl;

  string rtl_type = "RtlTcp";
  cfg.getValue(name, "TYPE", rtl_type);
  if (rtl_type == "RtlTcp")
  {
    string remote_host = "localhost";
    cfg.getValue(name, "HOST", remote_host);
    int tcp_port = 1234;
    cfg.getValue(name, "PORT", tcp_port);

    //cout << "###   HOST        = " << remote_host << endl;
    //cout << "###   PORT        = " << tcp_port << endl;
    rtl = new RtlTcp(remote_host, tcp_port);
  }
#ifdef HAS_RTLSDR_SUPPORT
  else if (rtl_type == "RtlUsb")
  {
    string dev_match = "0";
    cfg.getValue(name, "DEV_MATCH", dev_match);
    //cout << "###   DEV_MATCH   = " << dev_match << endl;
    rtl = new RtlUsb(dev_match);
  }
#endif
  else
  {
    cerr << "*** ERROR: Unknown WbRx type: " << rtl_type << endl;
    exit(1);
  }

  int sample_rate = 960000;
  cfg.getValue(name, "SAMPLE_RATE", sample_rate);
  //cout << "###   SAMPLE_RATE = " << sample_rate << endl;
  rtl->setSampleRate(sample_rate);
  rtl->iqReceived.connect(iqReceived.make_slot());
  rtl->readyStateChanged.connect(
      mem_fun(*this, &WbRxRtlSdr::rtlReadyStateChanged));

  int fq_corr = 0;
  if (cfg.getValue(name, "FQ_CORR", fq_corr) && (fq_corr != 0))
  {
    //cout << "###   FQ_CORR     = " << (int32_t)fq_corr << "ppm\n";
    rtl->setFqCorr(fq_corr);
  }

  uint32_t center_fq = 0;
  if (cfg.getValue(name, "CENTER_FQ", center_fq))
  {
    //cout << "###   CENTER_FQ   = " << center_fq << "Hz\n";
    auto_tune_enabled = false;
    setCenterFq(center_fq);
  }

  cfg.getValue(name, "XVRTR_OFFSET", xvrtr_offset);

  float gain = 0.0f;
  if (cfg.getValue(name, "GAIN", gain))
  {
    //cout << "###   GAIN        = " << gain << "dB\n";
    rtl->setGainMode(1);
    int32_t int_gain = static_cast<int32_t>(10.0 * gain);
    rtl->setGain(int_gain);
  }

  bool peak_meter = false;
  cfg.getValue(name, "PEAK_METER", peak_meter);
  rtl->enableDistPrint(peak_meter);
} /* WbRxRtlSdr::WbRxRtlSdr */


WbRxRtlSdr::~WbRxRtlSdr(void)
{
  delete rtl;
  rtl = 0;
} /* WbRxRtlSdr::~WbRxRtlSdr */


void WbRxRtlSdr::setCenterFq(uint32_t fq)
{
  //cout << "### WbRxRtlSdr::setCenterFq: fq=" << fq << endl;
  rtl->setCenterFq(fq + xvrtr_offset);
  for (Ddrs::iterator it=ddrs.begin(); it!=ddrs.end(); ++it)
  {
    Ddr *ddr = (*it);
    ddr->tunerFqChanged(fq);
  }
} /* WbRxRtlSdr::setCenterFq */


uint32_t WbRxRtlSdr::centerFq(void)
{
  return rtl->centerFq() - xvrtr_offset;
} /* WbRxRtlSdr::centerFq */


uint32_t WbRxRtlSdr::sampleRate(void) const
{
  return rtl->sampleRate();
} /* WbRxRtlSdr::sampleRate */


void WbRxRtlSdr::registerDdr(Ddr *ddr)
{
  Ddrs::iterator it = ddrs.find(ddr);
  assert(it == ddrs.end());
  ddrs.insert(ddr);
  if (auto_tune_enabled)
  {
    findBestCenterFq();
  }
} /* WbRxRtlSdr::registerDdr */


void WbRxRtlSdr::unregisterDdr(Ddr *ddr)
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
    instances.erase(m_name);
    delete this;
  }
} /* WbRxRtlSdr::unregisterDdr */


void WbRxRtlSdr::updateDdrFq(Ddr *ddr)
{
  if (auto_tune_enabled)
  {
    findBestCenterFq();
  }
} /* WbRxRtlSdr::updateDdrFq */


bool WbRxRtlSdr::isReady(void) const
{
  return (rtl != 0) && rtl->isReady();
} /* WbRxRtlSdr::isReady */



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

void WbRxRtlSdr::findBestCenterFq(void)
{
  if (ddrs.empty())
  {
    return;
  }

    // Put all DDR frequencies in a list and sort it
  deque<uint32_t> fqs;
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
  uint32_t span = 0;
  while ((span = fqs.back() - fqs.front()) > (rtl->sampleRate()-25000))
  {
    cout << "### Span too wide: " << span << endl;
    uint32_t front_dist = *(fqs.begin()+1) - fqs.front();
    uint32_t back_dist = fqs.back() - *(fqs.rbegin()+1);
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
  uint32_t center_fq = (fqs.front() + fqs.back()) / 2;

    // Check if we have any channel too close to the center of the band.
    // The center of the band almost always contain interference signals.
    // Shift the band to get channels away from the center.
  deque<uint32_t>::iterator it = fqs.begin();
  while (it != fqs.end())
  {
    deque<uint32_t>::iterator next = it+1;
    int32_t dist = *it - center_fq;
    int32_t next_dist = *next - center_fq;
    if ((next == fqs.end()) || (abs(dist) < abs(next_dist)))
    {
      if (abs(dist) < 12500)
      {
        int32_t max_move = (rtl->sampleRate() - span) / 2;
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

} /* WbRxRtlSdr::findBestCenterFq */


void WbRxRtlSdr::rtlReadyStateChanged(void)
{
  cout << name() << ": ";
  if (rtl->isReady())
  {
    cout << "Changed state to READY\n";

    cout << "\tUsing device      : " << rtl->displayName() << endl;
    cout << "\tTuner type        : " << rtl->tunerTypeString() << endl;
    vector<int> int_tuner_gains = rtl->getTunerGains();
    vector<float> tuner_gains;
    tuner_gains.assign(int_tuner_gains.begin(), int_tuner_gains.end());
    transform(tuner_gains.begin(), tuner_gains.end(),
        tuner_gains.begin(), bind2nd(divides<float>(),10.0));
    cout << "\tValid tuner gains : ";
    copy(tuner_gains.begin(), tuner_gains.end(),
        ostream_iterator<float>(cout, " "));
    cout << endl;
  }
  else
  {
    cout << "Changed state to NOT READY\n";
  }

  readyStateChanged();
} /* WbRxRtlSdr::rtlReadyStateChanged */



/*
 * This file has not been truncated
 */

