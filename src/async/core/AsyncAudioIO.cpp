/**
@file	 AsyncAudioIO.cpp
@brief   Contains a class for handling audio input/output to an audio device
@author  Tobias Blomberg
@date	 2003-03-23

This file contains a class for handling audio input and output to an audio
device. See usage instruction in the class documentation.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003  Tobias Blomberg

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

#include <linux/soundcard.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <cstdio>
#include <cassert>
#include <cerrno>


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

#include "AsyncFdWatch.h"
#include "AsyncAudioIO.h"



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


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
AudioIO::AudioIO(void)
  : fd(-1), mode(MODE_NONE), read_watch(0), write_watch(0), file(-1),
    old_mode(MODE_NONE), write_file_flush_timer(0)
{

} /* AudioIO::AudioIO */


AudioIO::~AudioIO(void)
{
  close();
} /* AudioIO::~AudioIO */


bool AudioIO::isFullDuplexCapable(void)
{
  if (!open(MODE_RD))
  {
    return false;
  }
  
  int caps;
  if (ioctl(fd, SNDCTL_DSP_GETCAPS, &caps) == -1)
  {
    perror("SNDCTL_DSP_SETFRAGMENT ioctl failed");
    close();
    return false;
  }
  
  close();
  
  if (!(caps & DSP_CAP_DUPLEX))
  {
    return false;
  }
  
  return true;
  
} /* AudioIO::isFullDuplexCapable */


bool AudioIO::open(Mode mode)
{
  if (fd != -1)
  {
    close();
  }
  
  int flags = O_NONBLOCK;
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
  
  fd = ::open("/dev/dsp", flags);
  if(fd < 0)
  {
    perror("open in AudioIO::open");
    return false;
  }

  int arg  = (FRAG_COUNT << 16) | FRAG_SIZE_LOG2;
  if (ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &arg) == -1)
  {
    perror("SNDCTL_DSP_SETFRAGMENT ioctl failed");
    close();
    return false;
  }

  arg = SIZE;
  if(ioctl(fd, SOUND_PCM_WRITE_BITS, &arg) == -1)
  {
    perror("SOUND_PCM_WRITE_BITS ioctl failed");
    close();
    return false;
  }
  if(arg != SIZE)
  {
    perror("unable to set sample size");
    close();
    return false;
  }

  arg = CHANNELS;
  if(ioctl(fd, SOUND_PCM_WRITE_CHANNELS, &arg) == -1)
  {
    perror("SOUND_PCM_WRITE_CHANNELS ioctl failed");
    close();
    return false;
  }
  if(arg != CHANNELS)
  {
    perror("unable to set number of channels");
    close();
    return false;
  }

  arg = RATE;
  if(ioctl(fd, SOUND_PCM_WRITE_RATE, &arg) == -1)
  {
    perror("SOUND_PCM_WRITE_RATE ioctl failed");
    close();
    return false;
  }
  
  arg = AFMT_S16_LE; 
  if(ioctl(fd,  SOUND_PCM_SETFMT, &arg) == -1)
  {
    perror(" SOUND_PCM_SETFMT ioctl failed");
    close();
    return false;
  }
  
  this->mode = mode;
  
  arg = 0;
  if ((mode == MODE_RD) || (mode == MODE_RDWR))
  {
    read_watch = new FdWatch(fd, FdWatch::FD_WATCH_RD);
    assert(read_watch != 0);
    read_watch->activity.connect(slot(this, &AudioIO::audioReadHandler));
    arg |= PCM_ENABLE_INPUT;
  }
  
  if ((mode == MODE_WR) || (mode == MODE_RDWR))
  {
    arg |= PCM_ENABLE_OUTPUT;
  }
  
  if(ioctl(fd, SNDCTL_DSP_SETTRIGGER, &arg) == -1)
  {
    perror("SNDCTL_DSP_SETTRIGGER ioctl failed");
    close();
    return false;
  }
  
  write_watch = new FdWatch(fd, FdWatch::FD_WATCH_WR);
  assert(write_watch != 0);
  write_watch->activity.connect(slot(this, &AudioIO::writeSpaceAvailable));
  write_watch->setEnabled(false);
  
  return true;
  
} /* AudioIO::open */


void AudioIO::close(void)
{
  mode = MODE_NONE;
  
  delete write_watch;
  write_watch = 0;
  
  delete read_watch;
  read_watch = 0;
  
  if (fd != -1)
  {
    ::close(fd);
    fd = -1;
  }
} /* AudioIO::close */


int AudioIO::write(const short *buf, int count)
{
  assert(fd >= 0);
  /*
  assert((mode == MODE_WR) || (mode == MODE_RDWR));
  */
  
  if (file != -1)
  {
    return count;
  }
  
    /* FIXME: How should we handle this case ? */
  if ((mode != MODE_WR) && (mode != MODE_RDWR))
  {
    return 0;
  }
  
  int written = ::write(fd, buf, sizeof(*buf) * count);
  if (written == -1)
  {
    if (errno == EAGAIN)
    {
      writeBufferFull(true);
      write_watch->setEnabled(true);
      return 0;
    }
    else
    {
      perror("write in AudioIO::write");
      return -1;
    }
  }
  
  return written / 2;
  
} /* AudioIO::write */


bool AudioIO::writeFile(const string& filename)
{
  old_mode = mode;
  if ((mode == MODE_RD) || (mode == MODE_NONE))
  {
    close();
    if (!open(MODE_WR))
    {
      open(old_mode);
      old_mode = MODE_NONE;
      return false;
    }
  }
  
  file = ::open(filename.c_str(), O_RDONLY);
  if (file != -1)
  {
    writeFromFile();
  }
  else
  {
    open(old_mode);
    old_mode = MODE_NONE;
    return false;
  }
  
  return true;
  
} /* AudioIO::playFile */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void AudioIO::audioReadHandler(FdWatch *watch)
{
  char buf[1280];
  int cnt = read(fd, buf, sizeof(buf));
  if (cnt == -1)
  {
    perror("read in AudioIO::audioReadHandler");
    return;
  }
  
  audioRead(reinterpret_cast<short *>(buf), cnt/sizeof(short));
  
} /* AudioIO::audioReadHandler */


void AudioIO::writeSpaceAvailable(FdWatch *watch)
{
  if (file != -1)
  {
    writeFromFile();
  }
  else
  {
    watch->setEnabled(false);
    writeBufferFull(false);
  }
} /* AudioIO::writeSpaceAvailable */


void AudioIO::writeFromFile(void)
{
  short buf[160];
  assert(file != 0);
  bool success = false;
  
  int written;
  int read_cnt;
  do
  {
    read_cnt = read(file, buf, sizeof(buf));
    if (read_cnt == -1)
    {
      perror("read in AudioIO::writeFromFile");
      goto done;
    }
    else if (read_cnt == 0)
    {
      success = true;
      goto done;
    }
    
    written = ::write(fd, buf, read_cnt);
    if (written == -1)
    {
      if (errno == EAGAIN)
      {
	written = 0;
	break;
      }
      else
      {
	perror("write in AudioIO::writeFromFile");
	goto done;
      }
    }
  } while (written == read_cnt);
  
  lseek(file, written - read_cnt, SEEK_CUR);
  write_watch->setEnabled(true);
  
  return;
    
  done:
    write_watch->setEnabled(false);
    ::close(file);
    file = -1;
    
    audio_buf_info info;
    if (ioctl(fd, SNDCTL_DSP_GETOSPACE, &info) == -1)
    {
      perror("SNDCTL_DSP_GETOSPACE ioctl failed");
      writeFileDone(0);
      open(old_mode);
      return;
    }
    long bytes_to_flush = info.fragsize * (info.fragstotal - info.fragments);
    long flushtime = 1000 * (bytes_to_flush / sizeof(short)) / 8000;
    write_file_flush_timer = new Timer(flushtime);
    write_file_flush_timer->expired.connect(
	slot(this, &AudioIO::writeFileDone));
    return;
    
} /* AudioIO::writeFromFile */


void AudioIO::writeFileDone(Timer *timer)
{
  if (old_mode != mode)
  {
    open(old_mode);
  }
  old_mode = MODE_NONE;
  fileWritten(timer != 0);
  delete write_file_flush_timer;
  write_file_flush_timer = 0;
} /* AudioIO::writeFileDone */



/*
 * This file has not been truncated
 */

