/**
@file	 AsyncAudioDevice.cpp
@brief   Handle OSS audio devices
@author  Tobias Blomberg
@date	 2004-03-20

This file contains an implementation of a class that handles OSS audio
devices.

\verbatim
Async - A library for programming event driven applications
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

#include <cassert>
#include <algorithm>


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

#include "SampleFifo.h"
#include "AsyncFdWatch.h"
#include "AsyncAudioIO.h"
#include "AsyncAudioDevice.h"



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

map<string, AudioDevice*>  AudioDevice::devices;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

AudioDevice *AudioDevice::registerAudioIO(const string& dev_name,
      	AudioIO *audio_io)
{
  if (devices.count(dev_name) == 0)
  {
    devices[dev_name] = new AudioDevice(dev_name);
    
      // Open the device once to read out the device capabilities
    if (devices[dev_name]->open(MODE_RDWR))
    {
      devices[dev_name]->close();
    }
  }
  AudioDevice *dev = devices[dev_name];
  ++dev->use_count;
  dev->aios.push_back(audio_io);
  return dev;
  
} /* AudioDevice::registerAudioIO */


void AudioDevice::unregisterAudioIO(AudioIO *audio_io)
{
  AudioDevice *dev = audio_io->device();
  assert(dev->use_count > 0);
  
  list<AudioIO*>::iterator it =
      	  find(dev->aios.begin(), dev->aios.end(), audio_io);
  assert(it != dev->aios.end());
  dev->aios.erase(it);
  
  if (--dev->use_count == 0)
  {
    devices.erase(dev->dev_name);
    delete dev;
  }
} /* AudioDevice::unregisterAudioIO */


bool AudioDevice::isFullDuplexCapable(void)
{
  return (device_caps & DSP_CAP_DUPLEX);
} /* AudioDevice::isFullDuplexCapable */


bool AudioDevice::open(Mode mode)
{
  if (mode == current_mode) // Same mode => do nothing
  {
    return true;
  }
  
  if (mode == MODE_NONE)  // Same as calling close
  {
    close();
  }
  
  if (current_mode == MODE_RDWR)  // Already RDWR => don't have to do anything
  {
    return true;
  }
  
     // Is not closed and is either read or write and we want the other
  if ((current_mode != MODE_NONE) && (mode != current_mode))
  {
    mode = MODE_RDWR;
  }
  
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
    perror("open failed");
    return false;
  }

  if (mode == MODE_RDWR)
  {
    ioctl(fd, SNDCTL_DSP_SETDUPLEX, 0);
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
  
  arg  = (FRAG_COUNT << 16) | FRAG_SIZE_LOG2;
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
  
  arg = CHANNELS;
  if(ioctl(fd, SNDCTL_DSP_CHANNELS, &arg) == -1)
  {
    perror("SNDCTL_DSP_CHANNELS ioctl failed");
    close();
    return false;
  }
  if(arg != CHANNELS)
  {
    fprintf(stderr, "*** error: Unable to set number of channels to %d. The "
      	      	    "driver suggested %d channels\n",
		    CHANNELS, arg);
    close();
    return false;
  }

  arg = RATE;
  if(ioctl(fd, SNDCTL_DSP_SPEED, &arg) == -1)
  {
    perror("SNDCTL_DSP_SPEED ioctl failed");
    close();
    return false;
  }
  if (abs(arg - RATE) > 100)
  {
    fprintf(stderr, "*** error: Sampling speed could not be set to %dHz. "
      	      	    "The closest speed returned by the driver was %dHz\n",
		    RATE, arg);
    close();
    return false;
  }
  //printf("Reported sampling rate: %dHz\n", arg);
  
  current_mode = mode;
  
  arg = 0;
  if ((mode == MODE_RD) || (mode == MODE_RDWR))
  {
    read_watch = new FdWatch(fd, FdWatch::FD_WATCH_RD);
    assert(read_watch != 0);
    read_watch->activity.connect(slot(*this, &AudioDevice::audioReadHandler));
    arg |= PCM_ENABLE_INPUT;
  }
  
  if ((mode == MODE_WR) || (mode == MODE_RDWR))
  {
    write_watch = new FdWatch(fd, FdWatch::FD_WATCH_WR);
    assert(write_watch != 0);
    write_watch->activity.connect(
      	    slot(*this, &AudioDevice::writeSpaceAvailable));
    //printf("%d: watch->setEnabled(false)\n", __LINE__);
    //write_watch->setEnabled(false);
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
      
  int frag_size = 0;
  if (ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &frag_size) == -1)
  {
    perror("SNDCTL_DSP_GETBLKSIZE ioctl failed");
    close();
    return false;
  }
  //printf("frag_size=%d\n", frag_size);
  
  if (read_buf == 0)
  {
    read_buf = new int16_t[BUF_FRAG_COUNT*frag_size];
    samples = new float[BUF_FRAG_COUNT*frag_size];
    last_frag = new int16_t[frag_size];
    memset(last_frag, 0, frag_size * sizeof(*last_frag));
  }
  
  return true;
    
} /* AudioDevice::open */


void AudioDevice::close(void)
{
  list<AudioIO*>::iterator it;
  for (it=aios.begin(); it!=aios.end(); ++it)
  {
    if ((*it)->mode() != AudioIO::MODE_NONE)
    {
      return;
    }
  }
  closeDevice();
} /* AudioDevice::close */


void AudioDevice::audioToWriteAvailable(void)
{
  //printf("AudioDevice::audioToWriteAvailable\n");
  
  /*
  if (write_watch->isEnabled())
  {
    return;
  }
  */
  
  //printf("%d: watch->setEnabled(true)\n", __LINE__);
  write_watch->setEnabled(true);
  
} /* AudioDevice::audioToWriteAvailable */


void AudioDevice::flushSamples(void)
{
  //printf("AudioDevice::flushSamples\n");
  prebuf = false;
  if (write_watch != 0)
  {
    //printf("%d: watch->setEnabled(true)\n", __LINE__);
    write_watch->setEnabled(true);
  }
} /* AudioDevice::flushSamples */


int AudioDevice::samplesToWrite(void) const
{
  if ((current_mode != MODE_WR) && (current_mode != MODE_RDWR))
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
         (sizeof(int16_t) * CHANNELS);
  
} /* AudioDevice::samplesToWrite */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


AudioDevice::AudioDevice(const string& dev_name)
  : dev_name(dev_name), use_count(0), current_mode(MODE_NONE), fd(-1),
    read_watch(0), write_watch(0), read_buf(0), device_caps(0), prebuf(true),
    samples(0), last_frag(0), use_fillin(false)
{
  char *use_trigger_str = getenv("ASYNC_AUDIO_NOTRIGGER");
  use_trigger = (use_trigger_str != 0) && (atoi(use_trigger_str) == 0);
} /* AudioDevice::AudioDevice */


AudioDevice::~AudioDevice(void)
{
  delete [] read_buf;
  read_buf = 0;
  
  delete [] samples;
  samples = 0;
  
  delete [] last_frag;
  last_frag = 0;
    
} /* AudioDevice::~AudioDevice */






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


void AudioDevice::audioReadHandler(FdWatch *watch)
{
  audio_buf_info info;
  if (ioctl(fd, SNDCTL_DSP_GETISPACE, &info) == -1)
  {
    perror("SNDCTL_DSP_GETISPACE ioctl failed");
    return;
  }
  //printf("read: fragments=%d  fragstotal=%d  fragsize=%d  bytes=%d\n",
    //  	 info.fragments, info.fragstotal, info.fragsize, info.bytes);
  
  if (info.fragments > 0)
  {
    int frags_to_read = info.fragments > BUF_FRAG_COUNT ?
      	    BUF_FRAG_COUNT : info.fragments;
    int cnt = read(fd, read_buf, frags_to_read * info.fragsize);
    if (cnt == -1)
    {
      perror("read in AudioDevice::audioReadHandler");
      return;
    }
    cnt /= sizeof(int16_t); // Convert cnt to number of samples
    
    //printf("Read %d samples\n", cnt);
    
    for (int ch=0; ch<CHANNELS; ++ch)
    {
      for (int i=ch; i<cnt; i += CHANNELS)
      {
	samples[i/CHANNELS] = static_cast<float>(read_buf[i]) / 32768.0;
      }

      list<AudioIO*>::iterator it;
      for (it=aios.begin(); it!=aios.end(); ++it)
      {
	if ((*it)->channel() == ch)
	{
      	  (*it)->audioRead(samples, cnt / CHANNELS);
	}
      }
    }
    //audioRead(samples, cnt/sizeof(int16_t));
  }
    
} /* AudioDevice::audioReadHandler */


void AudioDevice::writeSpaceAvailable(FdWatch *watch)
{
  //printf("AudioDevice::writeSpaceAvailable\n");
  
  //writeBufferFull(false);
  
  assert(fd >= 0);
  assert((current_mode == MODE_WR) || (current_mode == MODE_RDWR));
  
  unsigned samples_to_write ;
  audio_buf_info info;
  bool do_flush;
  unsigned fragsize;
  unsigned fragments;
  do
  {
    int16_t buf[32768];
    memset(buf, 0, sizeof(buf));

    if (ioctl(fd, SNDCTL_DSP_GETOSPACE, &info) == -1)
    {
      perror("SNDCTL_DSP_GETOSPACE ioctl failed");
      return;
    }
    //printf("write: fragments=%d  fragstotal=%d  fragsize=%d  bytes=%d\n",
      //	   info.fragments, info.fragstotal, info.fragsize, info.bytes);  

    /*
    int frags_to_write = sizeof(*buf) * count / info.fragsize;
    */
    //samples_to_write = min(samples_to_write, info.bytes / sizeof(int16_t));
    
    //fragments = max(0, 2 - (info.fragstotal - info.fragments));
    fragments = info.fragments;
    fragsize = info.fragsize / sizeof(int16_t);
    samples_to_write = min(static_cast<unsigned>(sizeof(buf) / sizeof(*buf)),
    		fragments * fragsize);
    //samples_to_write = min(samples_to_write, 7936U);
    
    //printf("audio device buffer=%d\n", info.fragments * fragsize);
    
    if (samples_to_write == 0)
    {
      break;
    }
    
    list<AudioIO*>::iterator it;
    do_flush = true;
    unsigned int max_samples_in_fifo = 0;
    for (it=aios.begin(); it!=aios.end(); ++it)
    {
      if (((*it)->mode() == AudioIO::MODE_WR) ||
      	  ((*it)->mode() == AudioIO::MODE_RDWR))
      {
	SampleFifo &fifo = (*it)->writeFifo();
	do_flush &= ((*it)->doFlush() &&
		     (fifo.samplesInFifo(true) <= samples_to_write));
	if (!(*it)->doFlush())
	{
	  samples_to_write = min(samples_to_write,
	      	      	      	 fifo.samplesInFifo(true) * CHANNELS);
	}
	max_samples_in_fifo = max(max_samples_in_fifo,
	      	      	      	  fifo.samplesInFifo(true));
      }
    }
    samples_to_write = min(samples_to_write, max_samples_in_fifo * CHANNELS);
    //printf("Samples to write from FIFO=%u. do_flush=%s\n", samples_to_write,
    //		do_flush ? "true" : "false");
    
    if (!do_flush)
    {
      samples_to_write /= fragsize;
      samples_to_write *= fragsize;
      
      if (prebuf && (samples_to_write < 2 * fragsize))
      {
      	if (fragments < 2)
	{
	  break;
	}
	else
	{
	  //printf("%d: watch->setEnabled(false)\n", __LINE__);
      	  watch->setEnabled(false);
	}
	return;
      }
    }
    prebuf = do_flush;
    //printf("prebuf=%s\n", prebuf ? "TRUE" : "FALSE");
    	
    if (samples_to_write == 0)
    {
      if (use_fillin)
      {
	//printf("Injecting fillin frag\n");
      	memcpy(buf, last_frag, fragsize * sizeof(*buf));
	samples_to_write = fragsize;
      }
      else
      {
	//printf("%d: watch->setEnabled(false)\n", __LINE__);
      	watch->setEnabled(false);
      	return;
      }
    }
    else
    {
      for (it=aios.begin(); it!=aios.end(); ++it)
      {
	if (((*it)->mode() == AudioIO::MODE_WR) ||
      	    ((*it)->mode() == AudioIO::MODE_RDWR))
	{
	  int channel = (*it)->channel();
	  float tmp[sizeof(buf)/sizeof(*buf)];
	  int samples_read =
	      	  (*it)->readSamples(tmp, samples_to_write / CHANNELS);
	  for (int i=0; i<samples_read; ++i)
	  {
	    int buf_pos = i * CHANNELS + channel;
	    float sample = 32767.0 * tmp[i] + buf[buf_pos];
	    if (sample > 32767)
	    {
	      buf[buf_pos] = 32767;
	    }
	    else if (sample < -32767)
	    {
	      buf[buf_pos] = -32767;
	    }
	    else
	    {
      	      buf[buf_pos] = static_cast<int16_t>(sample);
	    }
	  }
	}
      }  
    }
        
    if (do_flush && (samples_to_write % fragsize > 0))
    {
      //printf("*** FLUSHING %d samples ***\n", samples_to_write);
      samples_to_write /= fragsize;
      samples_to_write = (samples_to_write + 1) * fragsize;
    }
    
    //printf("Writing %d samples to audio device...\n", samples_to_write);
    int written = ::write(fd, buf, samples_to_write * sizeof(*buf));
    if (written == -1)
    {
      perror("write in AudioIO::write");
      return;
    }
    
    assert(written / sizeof(*buf) == samples_to_write);

    if (use_fillin)
    {
      if (do_flush)
      {
	memset(last_frag, 0, fragsize * sizeof(*last_frag));
      }
      else
      {
	int frags_written = samples_to_write / fragsize;
	memcpy(last_frag, buf + (frags_written - 1) * fragsize,
               fragsize * sizeof(*last_frag));
      }
    }
        
    /*
    if (do_flush)
    {
      printf("*** FLUSHING ***\n");
      if (ioctl(fd, SNDCTL_DSP_POST, NULL) == -1)
      {
	perror("SNDCTL_DSP_POST ioctl failed");
	return;
      }
    }
    */
  } while(samples_to_write == fragments * fragsize);
  
  //printf("Enabling audio device file descriptor watch\n");
  //printf("%d: watch->setEnabled(true)\n", __LINE__);
  watch->setEnabled(true);
  
} /* AudioDevice::writeSpaceAvailable */


void AudioDevice::closeDevice(void)
{
  current_mode = MODE_NONE;
  
  delete write_watch;
  write_watch = 0;
  
  delete read_watch;
  read_watch = 0;
  
  /*
  delete [] read_buf;
  read_buf = 0;
  
  delete [] samples;
  samples = 0;
  */
  
  if (fd != -1)
  {
    ::close(fd);
    fd = -1;
  }
} /* AudioDevice::closeDevice */


/*
 * This file has not been truncated
 */

