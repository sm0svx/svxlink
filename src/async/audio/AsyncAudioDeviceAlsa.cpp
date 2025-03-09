/**
@file	 AsyncAudioDeviceAlsa.cpp
@brief   Handle Alsa audio devices
@author  Steve / DH1DM, Tobias Blomberg / SM0SVX
@date	 2009-07-21

Implements the low level interface to an Alsa audio device.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2025 Tobias Blomberg / SM0SVX

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

#include <sigc++/sigc++.h>
#include <poll.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include <cstring>


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

#include "AsyncAudioDeviceAlsa.h"
#include "AsyncAudioDeviceFactory.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace sigc;



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

class AudioDeviceAlsa::AlsaWatch : public sigc::trackable
{
  public:
    AlsaWatch(snd_pcm_t *pcm_handle) : pcm_handle(pcm_handle)
    {
      int nfds = snd_pcm_poll_descriptors_count(pcm_handle);
      pollfd pfds[nfds];
      snd_pcm_poll_descriptors(pcm_handle, pfds, nfds);
      for (int i = 0; i < nfds; i++)
      {
        if (pfds[i].events & POLLOUT)
        {
          FdWatch *watch = new FdWatch(pfds[i].fd, FdWatch::FD_WATCH_WR);
          watch->activity.connect(mem_fun(*this, &AlsaWatch::writeEvent));
          watch_list.push_back(watch);
        }
        if (pfds[i].events & POLLIN)
        {
          FdWatch *watch = new FdWatch(pfds[i].fd, FdWatch::FD_WATCH_RD);
          watch->activity.connect(mem_fun(*this, &AlsaWatch::readEvent));
          watch_list.push_back(watch);
        }
        pfd_map[pfds[i].fd] = pfds[i];
      }
    }
  
    ~AlsaWatch()
    {
      //activity.clear();
      std::list<FdWatch*>::const_iterator cii;
      for(cii = watch_list.begin(); cii != watch_list.end(); ++cii)
      {
        delete *cii;
      }
    }
    
    void setEnabled(bool enable)
    {
      std::list<FdWatch*>::const_iterator cii;
      for(cii = watch_list.begin(); cii != watch_list.end(); ++cii)
      {
        (*cii)->setEnabled(enable);
      }
    }
  
    sigc::signal<void, FdWatch*, unsigned short> activity;

  private:
    std::map<int, pollfd> pfd_map;
    std::list<FdWatch*> watch_list;
    snd_pcm_t *pcm_handle;

    void writeEvent(FdWatch *watch)
    {
      pollfd pfd = pfd_map[watch->fd()];
      pfd.revents = POLLOUT;
      unsigned short revents;
      snd_pcm_poll_descriptors_revents(pcm_handle, &pfd, 1, &revents);
      activity(watch, revents);
    }
    void readEvent(FdWatch *watch)
    {
      pollfd pfd = pfd_map[watch->fd()];
      pfd.revents = POLLIN;
      unsigned short revents;
      snd_pcm_poll_descriptors_revents(pcm_handle, &pfd, 1, &revents);
      activity(watch, revents);
    }
};


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

REGISTER_AUDIO_DEVICE_TYPE("alsa", AudioDeviceAlsa);


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

AudioDeviceAlsa::AudioDeviceAlsa(const std::string& dev_name)
  : AudioDevice(dev_name), play_block_size(0), play_block_count(0),
    rec_block_size(0), rec_block_count(0), play_handle(0), 
    rec_handle(0), play_watch(0), rec_watch(0), duplex(false),
    zerofill_on_underflow(true)
{
  assert(AudioDeviceAlsa_creator_registered);

  char *zerofill_str = getenv("ASYNC_AUDIO_ALSA_ZEROFILL");
  if (zerofill_str != 0)
  {
    istringstream(zerofill_str) >> zerofill_on_underflow;
  }

  snd_pcm_t *play, *capture;

    // Open the device to check its duplex capability
  if (snd_pcm_open(&play, dev_name.c_str(), SND_PCM_STREAM_PLAYBACK, 0) == 0)
  {
     // Further initialization is not required here, since the AudioDevice
     // creator function will open the audio device again to check the assigned
     // I/O parameters.
    if (snd_pcm_open(&capture, dev_name.c_str(), 
                     SND_PCM_STREAM_CAPTURE, 0) == 0)
    {
      snd_pcm_close(capture);
      duplex = true;
    }
    snd_pcm_close(play);
  }
} /* AudioDeviceAlsa::AudioDeviceAlsa */


AudioDeviceAlsa::~AudioDeviceAlsa(void)
{
  closeDevice();

    // FIXME: What if we have multiple Alsa devices open?
  snd_config_update_free_global();
} /* AudioDeviceAlsa::~AudioDeviceAlsa */


size_t AudioDeviceAlsa::readBlocksize(void)
{
  return rec_block_size;
} /* AudioDeviceAlsa::readBlocksize */


size_t AudioDeviceAlsa::writeBlocksize(void)
{
  return play_block_size;
} /* AudioDeviceAlsa::writeBlocksize */


bool AudioDeviceAlsa::isFullDuplexCapable(void)
{
  return duplex;
} /* AudioDeviceAlsa::isFullDuplexCapable */


void AudioDeviceAlsa::audioToWriteAvailable(void)
{
  //printf("AudioDeviceAlsa::audioToWriteAvailable\n");
  if (play_watch)
  {
    play_watch->setEnabled(true);
  }
} /* AudioDeviceAlsa::audioToWriteAvailable */


void AudioDeviceAlsa::flushSamples(void)
{
  if (play_watch)
  {
    play_watch->setEnabled(true);
  }  
} /* AudioDeviceAlsa::flushSamples */


int AudioDeviceAlsa::samplesToWrite(void) const
{
  if ((mode() != MODE_WR) && (mode() != MODE_RDWR))
  {
    return 0;
  }

  int space_avail = snd_pcm_avail_update(play_handle);
  if (space_avail < 0)
  {
    return 0;
  }
  int samples_to_write = (play_block_count * play_block_size) - space_avail;
  if (samples_to_write < 0)
  {
    return 0;
  }
  return samples_to_write;

} /* AudioDeviceAlsa::samplesToWrite */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

bool AudioDeviceAlsa::openDevice(Mode mode)
{
  closeDevice();

  if ((mode == MODE_WR) || (mode == MODE_RDWR))
  {
    int err = snd_pcm_open(&play_handle, dev_name.c_str(),
			   SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
    {
      cerr << "*** ERROR: Open playback audio device failed: "
	   << snd_strerror(err)
	   << endl;
      return false;
    }

    if (!initParams(play_handle))
    {
      closeDevice();
      return false;
    }

    if (!getBlockAttributes(play_handle, play_block_size, play_block_count))
    {
      closeDevice();
      return false;
    }

    play_watch = new AlsaWatch(play_handle);
    play_watch->activity.connect(
            mem_fun(*this, &AudioDeviceAlsa::writeSpaceAvailable));
    play_watch->setEnabled(true);

    if (!startPlayback(play_handle))
    {
      cerr << "*** ERROR: Start playback failed" << endl;
      closeDevice();
      return false;
    }
  }

  if ((mode == MODE_RD) || (mode == MODE_RDWR))
  {
    int err = snd_pcm_open (&rec_handle, dev_name.c_str(),
			    SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0)
    {
      cerr << "*** ERROR: Open capture audio device failed: "
	   << snd_strerror(err)
	   << endl;
      return false;
    }

    if (!initParams(rec_handle))
    {
      closeDevice();
      return false;
    }

    if (!getBlockAttributes(rec_handle, rec_block_size, rec_block_count))
    {
      closeDevice();
      return false;
    }

    rec_watch = new AlsaWatch(rec_handle);
    rec_watch->activity.connect(
            mem_fun(*this, &AudioDeviceAlsa::audioReadHandler));

    if (!startCapture(rec_handle))
    {
      cerr << "*** ERROR: Start capture failed" << endl;
      closeDevice();
      return false;
    }
  }

  return true;

} /* AudioDeviceAlsa::openDevice */


void AudioDeviceAlsa::closeDevice(void)
{
  if (play_handle != 0)
  {
    snd_pcm_close(play_handle);
    play_handle = 0;
    delete play_watch;
    play_watch = 0;
  }

  if (rec_handle != 0)
  {
    snd_pcm_close(rec_handle);
    rec_handle = 0;
    delete rec_watch;
    rec_watch = 0;
  }
} /* AudioDeviceAlsa::closeDevice */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


void AudioDeviceAlsa::audioReadHandler(FdWatch *watch, unsigned short revents)
{
  //std::cout << "### AudioDeviceAlsa::audioReadHandler" << std::endl;

  assert(rec_handle != 0);
  assert((mode() == MODE_RD) || (mode() == MODE_RDWR));
  
  if (!(revents & POLLIN))
  {
    return;
  }  

  const auto pcm_state = snd_pcm_state(rec_handle);
  //std::cout << "### AudioDeviceAlsa::writeSpaceAvailable: pcm_state="
  //          << pcm_state << std::endl;
  if ((pcm_state < 0) || (pcm_state == SND_PCM_STATE_DISCONNECTED))
  {
    setDeviceError();
    return;
  }

  snd_pcm_sframes_t frames_avail = snd_pcm_avail_update(rec_handle);
  if (frames_avail < 0)
  {
    if (!startCapture(rec_handle))
    {
      watch->setEnabled(false);
    }
    return;
  }

  //printf("### frames_avail=%d\n", frames_avail);

  if (static_cast<size_t>(frames_avail) >= rec_block_size)
  {
    frames_avail /= rec_block_size;
    frames_avail *= rec_block_size;

    int16_t buf[frames_avail * channels];
    memset(buf, 0, sizeof(buf));

    const auto frames_read = snd_pcm_readi(rec_handle, buf, frames_avail);
    if (frames_read < 0)
    {
      if (!startCapture(rec_handle))
      {
        setDeviceError();
        watch->setEnabled(false);
      }
      return;
    }
    assert(frames_read <= frames_avail);

    putBlocks(buf, frames_read);
  }
} /* AudioDeviceAlsa::audioReadHandler */


void AudioDeviceAlsa::writeSpaceAvailable(FdWatch *watch, unsigned short revents)
{
  //printf("### AudioDeviceAlsa::writeSpaceAvailable\n");
  
  assert(play_handle != 0);
  assert((mode() == MODE_WR) || (mode() == MODE_RDWR));

  if (!(revents & POLLOUT))
  {
    return;
  }

  const auto pcm_state = snd_pcm_state(play_handle);
  //std::cout << "### AudioDeviceAlsa::writeSpaceAvailable: pcm_state="
  //          << pcm_state << std::endl;
  if ((pcm_state < 0) || (pcm_state == SND_PCM_STATE_DISCONNECTED))
  {
    setDeviceError();
    return;
  }

  while (1)
  {
    snd_pcm_sframes_t space_avail = snd_pcm_avail_update(play_handle);

      // Bail out if there's an error
    if (space_avail < 0)
    {
      if (!startPlayback(play_handle))
      {
        setDeviceError();
        watch->setEnabled(false);
        return;
      }
      continue;
    }

    size_t blocks_to_read = static_cast<size_t>(space_avail) / play_block_size;
    if (blocks_to_read == 0)
    {
      //printf("No free blocks available in sound card buffer\n");
      return;
    }

    int16_t buf[space_avail * channels];
        
    int blocks_avail = getBlocks(buf, blocks_to_read);
    if (blocks_avail == 0) 
    {
      if (zerofill_on_underflow)
      {
        blocks_avail = 1;
        memset(buf, 0, blocks_avail * play_block_size);
      }
      else
      {
        watch->setEnabled(false);
        return;
      }
    }
    
    int frames_to_write = blocks_avail * play_block_size;
    int frames_written = snd_pcm_writei(play_handle, buf, frames_to_write);
    //printf("frames_avail=%d  blocks_avail=%d  blocks_gotten=%d "
    //       "frames_written=%d\n", (int)frames_avail, blocks_avail,
    //       blocks_gotten, (int)frames_written);
    if (frames_written < 0)
    {
      if (!startPlayback(play_handle))
      {
        setDeviceError();
        watch->setEnabled(false);
        return;
      }
      continue;
    }

    if (frames_written != frames_to_write)
    {
      cerr << "*** WARNING: Number of frames written to sound device "
           << devName()
           << " (" << frames_written << ") differs from what was expected ("
           << frames_to_write << "). Audio was probably lost.\n";
      return;
    }
    
    if (frames_to_write != space_avail)
    {
      return;
    }
  }
} /* AudioDeviceAlsa::writeSpaceAvailable */


bool AudioDeviceAlsa::initParams(snd_pcm_t *pcm_handle)
{
  snd_pcm_hw_params_t* hw_params = nullptr;

  int err = snd_pcm_hw_params_malloc (&hw_params);
  if (err < 0)
  {
    cerr << "*** ERROR: Allocate hardware parameter structure failed: "
	 << snd_strerror(err)
	 << endl;
    return false;
  }

  err = snd_pcm_hw_params_any (pcm_handle, hw_params);
  if (err < 0)
  {
    cerr << "*** ERROR: Initialize hardware parameter structure failed: "
	 << snd_strerror(err)
	 << endl;
    snd_pcm_hw_params_free (hw_params);
    return false;
  }

  err = snd_pcm_hw_params_set_access(pcm_handle, hw_params,
				     SND_PCM_ACCESS_RW_INTERLEAVED);
  if (err < 0)
  {
    cerr << "*** ERROR: Set access type failed: "
	 << snd_strerror(err)
	 << endl;
    snd_pcm_hw_params_free (hw_params);
    return false;
  }

  err = snd_pcm_hw_params_set_format(pcm_handle, hw_params,
				     SND_PCM_FORMAT_S16_LE);
  if (err < 0)
  {
    cerr << "*** ERROR: Set sample format failed: "
    	 << snd_strerror(err)
	 << endl;
    snd_pcm_hw_params_free (hw_params);
    return false;
  }

  unsigned int real_rate = sample_rate;
  err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &real_rate, 0);
  if (err < 0)
  {
    cerr << "*** ERROR: Set sample rate failed: "
	 << snd_strerror(err)
	 << endl;
    snd_pcm_hw_params_free (hw_params);
    return false;
  }

  if (::abs(static_cast<int>(real_rate) - sample_rate) > 100)
  {
    cerr << "*** ERROR: The sample rate could not be set to "
         << sample_rate << "Hz for ALSA device \"" << dev_name << "\". "
         << "The closest rate returned by the driver was "
         << real_rate << "Hz."
         << endl;
    snd_pcm_hw_params_free(hw_params);
    return false;
  }

  err = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, channels);
  if (err < 0)
  {
    cerr << "*** ERROR: Set channel count failed: "
	 << snd_strerror(err)
	 << endl;
    snd_pcm_hw_params_free (hw_params);
    return false;
  }

  snd_pcm_uframes_t period_size = block_size_hint;
  err = snd_pcm_hw_params_set_period_size_near(pcm_handle, hw_params,
					       &period_size, 0);
  if (err < 0)
  {
    cerr << "*** ERROR: Set period size failed: "
	 << snd_strerror(err)
	 << endl;
    snd_pcm_hw_params_free (hw_params);
    return false;
  }
  
  snd_pcm_uframes_t buffer_size = block_count_hint * block_size_hint;
  err = snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hw_params,
					       &buffer_size);
  if (err < 0)
  {
     cerr << "*** ERROR: Set buffer size failed: "
	 << snd_strerror(err)
	 << endl;
     snd_pcm_hw_params_free (hw_params);
     return false;
  }
  
  err = snd_pcm_hw_params(pcm_handle, hw_params);
  if (err < 0)
  {
    cerr << "*** ERROR: Set hardware parameters failed: "
	 << snd_strerror(err)
	 << endl;
    snd_pcm_hw_params_free (hw_params);
    return false;
  }

  snd_pcm_uframes_t ret_period_size, ret_buffer_size;
  snd_pcm_hw_params_get_period_size(hw_params, &ret_period_size, 0);
  snd_pcm_hw_params_get_buffer_size(hw_params, &ret_buffer_size);

  snd_pcm_hw_params_free(hw_params);


  snd_pcm_sw_params_t *sw_params;
  
  err = snd_pcm_sw_params_malloc(&sw_params);
  if (err < 0)
  {
    cerr << "*** ERROR: Allocate software parameter structure failed: "
	 << snd_strerror(err)
	 << endl;
    return false;
  }

  err = snd_pcm_sw_params_current(pcm_handle, sw_params);
  if (err < 0)
  {
    cerr << "*** ERROR: Initialize software parameter structure failed: "
	 << snd_strerror(err)
	 << endl;
    snd_pcm_sw_params_free (sw_params);
    return false;
  }

  err = snd_pcm_sw_params_set_start_threshold(pcm_handle, sw_params,
      (ret_buffer_size / ret_period_size - 1) * ret_period_size);
  if (err < 0)
  {
    cerr << "*** ERROR: Set start threshold failed: "
	 << snd_strerror(err)
	 << endl;
    snd_pcm_sw_params_free (sw_params);
    return false;
  }

  err = snd_pcm_sw_params_set_avail_min(pcm_handle, sw_params, ret_period_size);
  if (err < 0)
  {
    cerr << "*** ERROR: Set min_avail threshold failed: "
	 << snd_strerror(err)
	 << endl;
    snd_pcm_sw_params_free(sw_params);
    return false;
  }

  err = snd_pcm_sw_params(pcm_handle, sw_params);
  if (err < 0)
  {
    cerr << "*** ERROR: Set software parameters failed: "
	 << snd_strerror(err)
	 << endl;
    snd_pcm_sw_params_free (sw_params);
    return false;
  }
  
  snd_pcm_sw_params_free(sw_params);

  return true;
} /* AudioDeviceAlsa::initParams */


bool AudioDeviceAlsa::getBlockAttributes(snd_pcm_t *pcm_handle,
                                         size_t &block_size,
                                         size_t &block_count)
{
  snd_pcm_hw_params_t *hw_params;
  int err = snd_pcm_hw_params_malloc (&hw_params);
  if (err < 0)
  {
    cerr << "*** ERROR: Allocate hardware parameter structure failed: "
         << snd_strerror(err)
         << endl;
    return false;
  }
  err = snd_pcm_hw_params_current(pcm_handle, hw_params);
  if (err < 0)
  {
    cerr << "*** ERROR: Failed to read current hardware params: "
         << snd_strerror(err)
         << endl;
    return false;
  }
  snd_pcm_uframes_t ret_period_size, ret_buffer_size;
  err = snd_pcm_hw_params_get_period_size(hw_params, &ret_period_size, 0);
  if (err < 0)
  {
    cerr << "*** ERROR: Failed to get period size: "
         << snd_strerror(err)
         << endl;
    return false;
  }
  err = snd_pcm_hw_params_get_buffer_size(hw_params, &ret_buffer_size);
  if (err < 0)
  {
    cerr << "*** ERROR: Failed to get buffer size: "
         << snd_strerror(err)
         << endl;
    return false;
  }
  snd_pcm_hw_params_free(hw_params);
  block_size = ret_period_size;
  block_count = ret_buffer_size / ret_period_size;
  return true;
} /* AudioDeviceAlsa::getBlockAttributes */


bool AudioDeviceAlsa::startPlayback(snd_pcm_t *pcm_handle)
{
  //std::cout << "### AudioDeviceAlsa::startPlayback" << std::endl;
  int err = snd_pcm_prepare(pcm_handle);
  if (err < 0)
  {
    cerr << "*** ERROR: snd_pcm_prepare failed (unrecoverable error): "
         << snd_strerror(err)
         << endl;
    return false;
  }
  return true;
} /* AudioDeviceAlsa::startPlayback */


bool AudioDeviceAlsa::startCapture(snd_pcm_t *pcm_handle)
{
  //std::cout << "### AudioDeviceAlsa::startCapture" << std::endl;
  int err = snd_pcm_prepare(pcm_handle);
  if (err < 0)
  {
    cerr << "*** ERROR: snd_pcm_prepare failed (unrecoverable error): "
         << snd_strerror(err)
         << endl;
    return false;
  }
  err = snd_pcm_start(pcm_handle);
  if (err < 0)
  {
    cerr << "*** ERROR: snd_pcm_start failed (unrecoverable error): "
         << snd_strerror(err)
         << endl;
    return false;
  }
  return true;
} /* AudioDeviceAlsa::startCapture */


/*
 * This file has not been truncated
 */
