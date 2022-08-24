/**
@file	 AsyncAudioDeviceOSS.cpp
@brief   Handle OSS audio devices
@author  Tobias Blomberg
@date	 2004-03-20

This file contains an implementation of a class that handles OSS audio
devices.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2009 Tobias Blomberg / SM0SVX

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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>

#include <cassert>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <cstdio>



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

#include "AsyncAudioDeviceOSS.h"
#include "AsyncAudioDeviceFactory.h"



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

REGISTER_AUDIO_DEVICE_TYPE("oss", AudioDeviceOSS);



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

size_t AudioDeviceOSS::readBlocksize(void)
{
  assert(fd != -1);
  return frag_size / (channels * sizeof(int16_t));
} /* AudioDeviceOSS::readBlocksize */


size_t AudioDeviceOSS::writeBlocksize(void)
{
  assert(fd != -1);
  return frag_size / (channels * sizeof(int16_t));
} /* AudioDeviceOSS::writeBlocksize */


bool AudioDeviceOSS::isFullDuplexCapable(void)
{
  return (device_caps & DSP_CAP_DUPLEX);
} /* AudioDeviceOSS::isFullDuplexCapable */


void AudioDeviceOSS::audioToWriteAvailable(void)
{
  write_watch->setEnabled(true);
} /* AudioDeviceOSS::audioToWriteAvailable */


void AudioDeviceOSS::flushSamples(void)
{
  if (write_watch != 0)
  {
    write_watch->setEnabled(true);
  }
} /* AudioDeviceOSS::flushSamples */


int AudioDeviceOSS::samplesToWrite(void) const
{
  if ((mode() != MODE_WR) && (mode() != MODE_RDWR))
  {
    return 0;
  }
  
  audio_buf_info info;
  if (ioctl(fd, SNDCTL_DSP_GETOSPACE, &info) == -1)
  {
    perror("SNDCTL_DSP_GETOSPACE ioctl failed");
    return -1;
  }
  
  return (info.fragsize * (info.fragstotal - info.fragments)) /
         (sizeof(int16_t) * channels);
  
} /* AudioDeviceOSS::samplesToWrite */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


AudioDeviceOSS::AudioDeviceOSS(const string& dev_name)
  : AudioDevice(dev_name), fd(-1), read_watch(0), write_watch(0),
    device_caps(0), use_trigger(false), frag_size(0)
{
  assert(AudioDeviceOSS_creator_registered);

  char *use_trigger_str = getenv("ASYNC_AUDIO_NOTRIGGER");
  use_trigger = (use_trigger_str != 0) && (atoi(use_trigger_str) == 0);

    // Open the device to check its capabilities
  int f = ::open(dev_name.c_str(), O_RDWR);
  if (f >= 0)
  {
    if (ioctl(fd, SNDCTL_DSP_SETDUPLEX, 0) == -1)
    {
      perror("SNDCTL_DSP_SETDUPLEX ioctl failed");
    }
    if (ioctl(fd, SNDCTL_DSP_GETCAPS, &device_caps) == -1)
    {
      perror("SNDCTL_DSP_GETCAPS ioctl failed");
    }
    ::close(f);
  }
} /* AudioDeviceOSS::AudioDeviceOSS */


AudioDeviceOSS::~AudioDeviceOSS(void)
{
} /* AudioDeviceOSS::~AudioDeviceOSS */


bool AudioDeviceOSS::openDevice(Mode mode)
{
  int arg;
  
  if (fd != -1)
  {
    closeDevice();
  }
  
  int flags = 0; // = O_NONBLOCK; // Not supported according to the OSS docs
  switch (mode)
  {
    case MODE_RD:
      flags |= O_RDONLY;
      break;
    case MODE_WR:
      flags |= O_WRONLY;
      break;
    case MODE_RDWR:
      flags |= O_RDWR;
      break;
    case MODE_NONE:
      return true;
  }
  
  fd = ::open(dev_name.c_str(), flags);
  if(fd < 0)
  {
    perror("open audio device failed");
    return false;
  }

  if (mode == MODE_RDWR)
  {
    if (ioctl(fd, SNDCTL_DSP_SETDUPLEX, 0) == -1)
    {
      perror("SNDCTL_DSP_SETDUPLEX ioctl failed");
      close();
      return false;
    }
  }
  
  if (ioctl(fd, SNDCTL_DSP_GETCAPS, &device_caps) == -1)
  {
    perror("SNDCTL_DSP_GETCAPS ioctl failed");
    close();
    return false;
  }
  
  if (use_trigger && (device_caps & DSP_CAP_TRIGGER))
  {
    arg = ~(PCM_ENABLE_OUTPUT | PCM_ENABLE_INPUT);
    if(ioctl(fd, SNDCTL_DSP_SETTRIGGER, &arg) == -1)
    {
      perror("SNDCTL_DSP_SETTRIGGER ioctl failed");
      close();
      return false;
    }
  }
  
  size_t size = (block_size_hint == 0) ? 1 :
	        block_size_hint * channels * sizeof(int16_t);
  int frag_size_log2 = static_cast<int>(log2(size));
  arg  = static_cast<int>((block_count_hint << 16) | frag_size_log2);
  if (ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &arg) == -1)
  {
    perror("SNDCTL_DSP_SETFRAGMENT ioctl failed");
    close();
    return false;
  }
  
  arg = AFMT_S16_NE; 
  if(ioctl(fd,  SNDCTL_DSP_SETFMT, &arg) == -1)
  {
    perror("SNDCTL_DSP_SETFMT ioctl failed");
    close();
    return false;
  }
  if (arg != AFMT_S16_NE)
  {
    fprintf(stderr, "*** error: The sound device does not support 16 bit "
      	      	    "signed samples\n");
    close();
    return false;
  }
  
  arg = static_cast<int>(channels);
  if(ioctl(fd, SNDCTL_DSP_CHANNELS, &arg) == -1)
  {
    perror("SNDCTL_DSP_CHANNELS ioctl failed");
    close();
    return false;
  }
  if(arg != static_cast<int>(channels))
  {
    fprintf(stderr, "*** error: Unable to set number of channels to %zu. The "
      	      	    "driver suggested %d channels\n",
		    channels, arg);
    close();
    return false;
  }

  arg = sample_rate;
  if(ioctl(fd, SNDCTL_DSP_SPEED, &arg) == -1)
  {
    perror("SNDCTL_DSP_SPEED ioctl failed");
    close();
    return false;
  }
  if (abs(arg - sample_rate) > 100)
  {
    fprintf(stderr, "*** error: The sampling rate could not be set to %dHz "
                    "for OSS device %s. The closest rate returned by the "
		    "driver was %dHz\n",
		    sample_rate, dev_name.c_str(), arg);
    close();
    return false;
  }
  
  arg = 0;
  if ((mode == MODE_RD) || (mode == MODE_RDWR))
  {
    read_watch = new FdWatch(fd, FdWatch::FD_WATCH_RD);
    assert(read_watch != 0);
    read_watch->activity.connect(
        mem_fun(*this, &AudioDeviceOSS::audioReadHandler));
    arg |= PCM_ENABLE_INPUT;
  }
  
  if ((mode == MODE_WR) || (mode == MODE_RDWR))
  {
    write_watch = new FdWatch(fd, FdWatch::FD_WATCH_WR);
    assert(write_watch != 0);
    write_watch->activity.connect(
      	mem_fun(*this, &AudioDeviceOSS::writeSpaceAvailable));
    arg |= PCM_ENABLE_OUTPUT;
  }
  
  if (use_trigger && (device_caps & DSP_CAP_TRIGGER))
  {
    if(ioctl(fd, SNDCTL_DSP_SETTRIGGER, &arg) == -1)
    {
      perror("SNDCTL_DSP_SETTRIGGER ioctl failed");
      close();
      return false;
    }
  }
  
  arg = 0;
  if (ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &arg) == -1)
  {
    perror("SNDCTL_DSP_GETBLKSIZE ioctl failed");
    close();
    return false;
  }
  assert(arg >= 0);
  frag_size = static_cast<size_t>(arg);
  
  return true;
  
} /* AudioDeviceOSS::openDevice */


void AudioDeviceOSS::closeDevice(void)
{
  frag_size = 0;
  
  delete write_watch;
  write_watch = 0;
  
  delete read_watch;
  read_watch = 0;
  
  if (fd != -1)
  {
    ::close(fd);
    fd = -1;
  }
} /* AudioDeviceOSS::closeDevice */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


void AudioDeviceOSS::audioReadHandler(FdWatch *watch)
{
  audio_buf_info info;
  if (ioctl(fd, SNDCTL_DSP_GETISPACE, &info) == -1)
  {
    perror("SNDCTL_DSP_GETISPACE ioctl failed");
    return;
  }
  
  if (info.fragments > 0)
  {
    int frags_to_read = info.fragments;
    int bytes_to_read = frags_to_read * info.fragsize;
    int16_t buf[bytes_to_read / sizeof(int16_t)];
    int cnt = read(fd, buf, bytes_to_read);
    if (cnt == -1)
    {
      perror("read in AudioDeviceOSS::audioReadHandler");
      return;
    }
    assert(cnt == bytes_to_read);
    
    int frame_cnt = bytes_to_read / (channels * sizeof(int16_t));
    //printf("frags_to_read=%d  bytes_to_read=%d  frame_cnt=%d\n", frags_to_read, bytes_to_read, frame_cnt);
    putBlocks(buf, frame_cnt);
  }
} /* AudioDeviceOSS::audioReadHandler */


void AudioDeviceOSS::writeSpaceAvailable(FdWatch *watch)
{
  assert(fd >= 0);
  assert((mode() == MODE_WR) || (mode() == MODE_RDWR));
  
  audio_buf_info info;
  //unsigned fragsize; // The frag (block) size in frames
  unsigned fragments;
  size_t frags_read;
  do
  {
      // Find out how many frags we can write to the sound card
    if (ioctl(fd, SNDCTL_DSP_GETOSPACE, &info) == -1)
    {
      perror("SNDCTL_DSP_GETOSPACE ioctl failed");
      return;
    }
    fragments = info.fragments;
    //fragsize = info.fragsize / (sizeof(int16_t) * channels);
    
      // Bail out if there's no free frags
    if (fragments == 0)
    {
      break;
    }
    
    int16_t buf[32768];
    frags_read = getBlocks(buf, fragments);
    //printf("fragments=%u  frags_read=%zu\n", fragments, frags_read);
    if (frags_read == 0)
    {
      watch->setEnabled(false);
      return;
    }
    
      // Write the samples to the sound card
    //printf("Writing %zu fragments\n", frags_read);
    int written = ::write(fd, buf, frags_read * frag_size);
    if (written < 0)
    {
      perror("write in AudioIO::write");
      return;
    }
    
    assert(static_cast<unsigned>(written) == frags_read * frag_size);

  } while(frags_read == fragments);
  
  watch->setEnabled(true);
  
} /* AudioDeviceOSS::writeSpaceAvailable */



/*
 * This file has not been truncated
 */

