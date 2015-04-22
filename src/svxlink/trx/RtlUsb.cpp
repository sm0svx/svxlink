/**
@file	 RtlUsb.cpp
@brief   An interface class for communicating to a RTL dongle through USB
@author  Tobias Blomberg / SM0SVX
@date	 2014-07-16

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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

#include <cstring>
#include <cstdlib>
#include <iterator>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cassert>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>


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

#include "RtlUsb.h"



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

RtlUsb::RtlUsb(const std::string &match)
  : reconnect_timer(0, Timer::TYPE_ONESHOT), dev(NULL),
    sample_pipe_watch(0), buf(0), buf_pos(0), dev_match(match), dev_name("?")
{
  sample_pipe[0] = sample_pipe[1] = -1;
  buf = new char[2 * blockSize()];
  reconnect_timer.expired.connect(
      hide(mem_fun(*this, &RtlUsb::initializeDongle)));
} /* RtlUsb::RtlUsb */


RtlUsb::~RtlUsb(void)
{
  verboseClose();
  delete [] buf;
  buf = 0;
} /* RtlUsb::~RtlUsb */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void RtlUsb::handleSetTunerIfGain(uint16_t stage, int16_t gain)
{
  if (dev == NULL)
  {
    return;
  }

  int r = rtlsdr_set_tuner_if_gain(dev, stage, gain);
  if (r < 0)
  {
    cerr << "*** WARNING: Failed to set tuner IF gain.\n";
    verboseClose();
  }
  else
  {
    /*
    cerr << "### Tuner IF gain stage " << stage << " set to " 
         << (gain / 10.0) << "dB\n";
         */
  }
} /* RtlUsb::handleSetTunerIfGain */


void RtlUsb::handleSetCenterFq(uint32_t fq)
{
  if (dev == NULL)
  {
    return;
  }

  int r = rtlsdr_set_center_freq(dev, fq);
  if (r < 0)
  {
    cerr << "*** WARNING: Failed to set center freq.\n";
    verboseClose();
  }
  else
  {
    //cerr << "### Tuned to " << fq << "Hz\n";
  }
} /* RtlUsb::handleSetCenterFq */


void RtlUsb::handleSetSampleRate(uint32_t rate)
{
  delete [] buf;
  buf = new char[2 * blockSize()];

  if (dev == NULL)
  {
    return;
  }

  int r = rtlsdr_set_sample_rate(dev, rate);
  if (r < 0)
  {
    cerr << "*** WARNING: Failed to set sample rate.\n";
    verboseClose();
  }
  else
  {
    //cerr << "### Sample rate set to " << rate << "Hz\n";
  }
} /* RtlUsb::handleSetSampleRate */


void RtlUsb::handleSetGainMode(uint32_t mode)
{
  if (dev == NULL)
  {
    return;
  }

  if (mode == 0)
  {
      /* Enable automatic gain */
    int r = rtlsdr_set_tuner_gain_mode(dev, 0);
    if (r < 0)
    {
      cerr << "*** WARNING: Failed to enable automatic gain.\n";
      verboseClose();
    }
    else
    {
      //cerr << "### Enable automatic gain control\n";
    }
  }
  else
  {
      /* Enable manual gain */
    int r = rtlsdr_set_tuner_gain_mode(dev, 1);
    if (r < 0)
    {
      cerr << "*** WARNING: Failed to enable manual gain.\n";
      verboseClose();
    }
    else
    {
      //cerr << "### Enable manual gain control\n";
    }
  }
} /* RtlUsb::handleSetGainMode */


void RtlUsb::handleSetGain(int32_t gain)
{
  if (dev == NULL)
  {
    return;
  }

    /* Set the tuner gain */
  int r = rtlsdr_set_tuner_gain(dev, gain);
  if (r < 0)
  {
    cout << "*** WARNING: Failed to set tuner gain.\n";
    verboseClose();
  }
  else
  {
    //cout << "### Tuner gain set to " << (gain/10.0) << "dB\n";
  }
} /* RtlUsb::handleSetGain */


void RtlUsb::handleSetFqCorr(uint32_t corr)
{
  if (dev == NULL)
  {
    return;
  }

  int r = rtlsdr_set_freq_correction(dev, corr);
  if (r < 0)
  {
    cerr << "*** WARNING: Failed to set ppm error\n";
    verboseClose();
  }
  else
  {
    //cerr << "### Tuner error set to " << corr << " ppm\n";
  }
} /* RtlUsb::handleSetFqCorr */


void RtlUsb::handleEnableTestMode(bool enable)
{
  if (dev == NULL)
  {
    return;
  }
  int r = rtlsdr_set_testmode(dev, enable ? 1 : 0);
  if (r < 0)
  {
    cerr << "*** WARNING: Failed to set test mode\n";
    verboseClose();
  }
  else
  {
    //cerr << "### Test mode " << (enable ? "enabled" : "disabled") << endl;
  }
} /* RtlUsb::handleEnableTestMode */


void RtlUsb::handleEnableDigitalAgc(bool enable)
{
  if (dev == NULL)
  {
    return;
  }
  int r = rtlsdr_set_agc_mode(dev, enable ? 1 : 0);
  if (r < 0)
  {
    cerr << "*** WARNING: Failed to set digital AGC\n";
    verboseClose();
  }
  else
  {
    //cerr << "### Digital AGC " << (enable ? "enabled" : "disabled") << endl;
  }
} /* RtlUsb::handleEnableDigitalAgc */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void *RtlUsb::rtlReader(void *data)
{
  RtlUsb *rtl = reinterpret_cast<RtlUsb*>(data);
  assert(rtl != 0);
  //cout << "### blockSize()=" << rtl->blockSize() << endl;
  uint32_t buf_len = 16384 * (2 * rtl->blockSize() / 16384);
  int r = rtlsdr_read_async(rtl->dev, rtlsdrCallback, rtl, 0, buf_len);
  if (r != 0)
  {
    cerr << "*** WARNING: Failed to read samples from RTL dongle\n";
    close(rtl->sample_pipe[1]);
    rtl->sample_pipe[1] = -1;
  }
  return NULL;
} /* RtlUsb::rtlReader */


void RtlUsb::rtlsdrCallback(unsigned char *buf, uint32_t len, void *ctx)
{
  //cout << "### RtlUsb::rtlsdrCallback: len=" << len << endl;
  RtlUsb *rtl = reinterpret_cast<RtlUsb*>(ctx);
  ssize_t ret = write(rtl->sample_pipe[1], buf, len);
  if (ret != len)
  {
    cerr << "*** WARNING: Samples were lost while writing to RTL sample pipe\n";
    rtl->verboseClose();
  }
} /* RtlUsb::rtlsdrCallback */


void RtlUsb::rtlSamplesReceived(void)
{
  size_t read_len = 2 * blockSize() - buf_pos;
  ssize_t len = read(sample_pipe[0], buf + buf_pos, read_len);
  /*
  cout << "### read_len=" << read_len << " buf_pos=" << buf_pos
       << " len=" << len << endl;
  */
  if (len < 0)
  {
    cerr << "*** WARNING: Failed to read from RTL sample pipe\n";
  }
  else if (len == 0)
  {
    verboseClose();
    return;
  }
  buf_pos += len;

  if (buf_pos >= 2 * blockSize())
  {
    buf_pos = 0;
    complex<uint8_t> *samples = reinterpret_cast<complex<uint8_t>*>(buf);
    handleIq(samples, blockSize());
  }
} /* RtlUsb::rtlSamplesReceived */


void RtlUsb::initializeDongle(void)
{
  assert(dev == NULL);

    // Find the dongle using either the device index, serial number prefix or
    // serial number suffix, whatever matches.
  int dev_index = verboseDeviceSearch(dev_match.c_str());
  if (dev_index < 0)
  {
    verboseClose();
    return;
  }
  
    // Open the dongle
  rtlsdr_open(&dev, (uint32_t)dev_index);
  if (dev == NULL)
  {
    cerr << "Failed to open rtlsdr device #" << dev_index << endl;
    verboseClose();
    return;
  }

    // Write current settings to the dongle
  updateSettings();

    // The setter functions may close the device so we better check it
  if (dev == NULL)
  {
    verboseClose();
    return;
  }

    // Get tuner type
  setTunerType(TUNER_UNKNOWN);
  enum rtlsdr_tuner tuner_type = rtlsdr_get_tuner_type(dev);
  switch (tuner_type)
  {
    case RTLSDR_TUNER_E4000:
      setTunerType(TUNER_E4000);
      break;
    case RTLSDR_TUNER_FC0012:
      setTunerType(TUNER_FC0012);
      break;
    case RTLSDR_TUNER_FC0013:
      setTunerType(TUNER_FC0013);
      break;
    case RTLSDR_TUNER_FC2580:
      setTunerType(TUNER_FC2580);
      break;
    case RTLSDR_TUNER_R820T:
      setTunerType(TUNER_R820T);
      break;
    case RTLSDR_TUNER_R828D:
      setTunerType(TUNER_R828D);
      break;
    default:
      cerr << "*** WARNING: Unknown RTL tuner type found\n";
      break;
  }

    // Reset endpoint before we start reading from it (mandatory)
  int r = rtlsdr_reset_buffer(dev);
  if (r < 0)
  {
    cerr << "*** WARNING: Failed to reset buffers.\n";
    verboseClose();
    return;
  }

  r = pthread_create(&rtl_reader_thread, NULL, rtlReader, this);
  if (r != 0)
  {
    cerr << "*** ERROR: Failed to create RTL reader thread: "
         << strerror(r) << "\n";
    verboseClose();
    return;
  }

  r = pipe(sample_pipe);
  if (r != 0)
  {
    cerr << "*** ERROR: Failed to create RTL reader pipe\n";
    sample_pipe[0] = sample_pipe[1] = -1;
    verboseClose();
    return;
  }
  sample_pipe_watch = new FdWatch(sample_pipe[0], FdWatch::FD_WATCH_RD);
  sample_pipe_watch->activity.connect(
      hide(mem_fun(*this, &RtlUsb::rtlSamplesReceived)));

  char vendor[256], product[256], serial[256];
  r = rtlsdr_get_device_usb_strings(dev_index, vendor, product, serial);
  if (r != 0)
  {
    cerr << "*** ERROR: Failed to read RTL USB device name\n";
    verboseClose();
    return;
  }
  ostringstream ss;
  ss << "[" << dev_index << "] " << vendor << " " << product 
     << " SN:" << serial;
  dev_name = ss.str();
  //cout << "### Device name: " << dev_name << endl;

    // Stop the reconnect timer
  reconnect_timer.setEnable(false);

    // Tell the world that we are ready for operation
  readyStateChanged();
} /* RtlUsb::initializeDongle */


void RtlUsb::verboseClose(void)
{
  if (dev == NULL)
  {
    reconnect_timer.setTimeout(RECONNECT_INTERVAL);
    reconnect_timer.setEnable(true);
    return;
  }

  /*
  cout << "### Canceling...\n";
  int r = pthread_cancel(rtl_reader_thread);
  if (r != 0)
  {
    cerr << "*** WARNING: Failed to cancel the RTL reader thread\n";
  }
  */

  int r = rtlsdr_cancel_async(dev);
  if (r != 0)
  {
    cerr << "*** WARNING: Failed to cancel the RTL async reader\n";
  }

  r = pthread_join(rtl_reader_thread, NULL);
  if (r != 0)
  {
    cerr << "*** WARNING: Failed to join the RTL reader thread: "
         << strerror(r) << "\n";
  }

  delete sample_pipe_watch;
  sample_pipe_watch = 0;

  if (sample_pipe[0] >= 0)
  {
    r = close(sample_pipe[0]);
    if (r != 0)
    {
      cerr << "*** WARNING: Failed to close read end of RTL "
           << "sample reader pipe: " << strerror(errno) << "\n";
    }
  }
  if (sample_pipe[1] >= 0)
  {
    r = close(sample_pipe[1]);
    if (r != 0)
    {
      cerr << "*** WARNING: Failed to close write end of RTL "
           << "sample reader pipe: " << strerror(errno) << "\n";
    }
  }

  r = rtlsdr_close(dev);
  if (r < 0)
  {
    cerr << "*** WARNING: Failed to close RTL device.\n";
  }
  dev = NULL;

  reconnect_timer.setTimeout(RECONNECT_INTERVAL);
  reconnect_timer.setEnable(true);

  setTunerType(TUNER_UNKNOWN);

  readyStateChanged();
} /* RtlUsb::verboseClose */


int RtlUsb::verboseDeviceSearch(const char *s)
{
  int i, device_count, device, offset;
  char *s2;
  char vendor[256], product[256], serial[256];
  device_count = rtlsdr_get_device_count();
  if (!device_count)
  {
    cerr << "*** WARNING: No supported RTL devices found\n";
    return -1;
  }

    /* Does string look like raw id number */
  device = (int)strtol(s, &s2, 0);
  if (s2[0] == '\0' && device >= 0 && device < device_count)
  {
    /*
    cerr << "### Device index match. Using device " << device << ": "
         << rtlsdr_get_device_name((uint32_t)device) << endl;
    */
    return device;
  }

    /* Does string exact match a serial */
  for (i = 0; i < device_count; i++)
  {
    rtlsdr_get_device_usb_strings(i, vendor, product, serial);
    if (strcmp(s, serial) != 0)
    {
      continue;
    }
    device = i;
    /*
    cerr << "### Serial match. Using device " << device << ": "
         << rtlsdr_get_device_name((uint32_t)device) << endl;
    */
    return device;
  }

    /* Does string prefix match a serial */
  for (i = 0; i < device_count; i++)
  {
    rtlsdr_get_device_usb_strings(i, vendor, product, serial);
    if (strncmp(s, serial, strlen(s)) != 0)
    {
      continue;
    }
    device = i;
    /*
    cerr << "### Serial prefix match. Using device " << device << ": "
         << rtlsdr_get_device_name((uint32_t)device) << endl;
    */
    return device;
  }

    /* Does string suffix match a serial */
  for (i = 0; i < device_count; i++)
  {
    rtlsdr_get_device_usb_strings(i, vendor, product, serial);
    offset = strlen(serial) - strlen(s);
    if (offset < 0)
    {
      continue;
    }
    if (strncmp(s, serial+offset, strlen(s)) != 0)
    {
      continue;
    }
    device = i;
    /*
    cerr << "### Serial suffix match. Using device " << device << ": " 
         << rtlsdr_get_device_name((uint32_t)device) << endl;
    */
    return device;
  }
  cerr << "*** WARNING: No RTL devices matching \"" << s << "\" found. "
          "The following devices has been detected:\n";
  for (i = 0; i < device_count; i++)
  {
    rtlsdr_get_device_usb_strings(i, vendor, product, serial);
    cerr << "      [" << i << "] " << vendor << " " << product
         << " SN:" << serial << endl;
  }
  return -1;
}



/*
 * This file has not been truncated
 */
