/**
@file	 MsgHandler.cpp
@brief   Handling of playback of audio clips
@author  Tobias Blomberg / SM0SVX
@date	 2005-10-22

This file contains an object that handles the playback of audio clips.

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003 Tobias Blomberg / SM0SVX

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
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

extern "C" {
#include <gsm.h>
}

#include <cassert>
#include <iostream>



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

#include "MsgHandler.h"



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

#define WRITE_BLOCK_SIZE    4*160



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

class QueueItem
{
  public:
    QueueItem(bool idle_marked) : idle_marked(idle_marked) {}
    virtual ~QueueItem(void) {}
    virtual bool initialize(void) { return true; }
    virtual int readSamples(float *samples, int len) = 0;
    virtual void unreadSamples(int len) = 0;
    
    bool idleMarked(void) const { return idle_marked; }
  
  private:
    bool  idle_marked;
};

class SilenceQueueItem : public QueueItem
{
  public:
    SilenceQueueItem(int len, int sample_rate, bool idle_marked)
      : QueueItem(idle_marked), len(len),
      	silence_left(sample_rate * len / 1000) {}
    int readSamples(float *samples, int len);
    void unreadSamples(int len);

  private:
    int len;
    int silence_left;
    
};

class ToneQueueItem : public QueueItem
{
  public:
    ToneQueueItem(int fq, int amp, int len, int sample_rate, bool idle_marked)
      : QueueItem(idle_marked), fq(fq), amp(amp),
      	tone_len(sample_rate * len / 1000), pos(0), sample_rate(sample_rate) {}
    int readSamples(float *samples, int len);
    void unreadSamples(int len);

  private:
    int fq;
    int amp;
    int tone_len;
    int pos;
    int sample_rate;
    
};

class RawFileQueueItem : public QueueItem
{
  public:
    RawFileQueueItem(const std::string& filename, bool idle_marked)
      : QueueItem(idle_marked), filename(filename), file(-1) {}
    ~RawFileQueueItem(void);
    bool initialize(void);
    int readSamples(float *samples, int len);
    void unreadSamples(int len);

  private:
    string  filename;
    int     file;
    
};

class GsmFileQueueItem : public QueueItem
{
  public:
    GsmFileQueueItem(const std::string& filename, bool idle_marked)
      : QueueItem(idle_marked), filename(filename), file(-1), decoder(0) {}
    ~GsmFileQueueItem(void);
    bool initialize(void);
    int readSamples(float *samples, int len);
    void unreadSamples(int len);

  private:
    static const int BUFSIZE = 160;
    
    string    	filename;
    int       	file;
    gsm       	decoder;
    int       	buf_pos;
    gsm_signal  buf[BUFSIZE];

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



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

MsgHandler::MsgHandler(int sample_rate)
  : sample_rate(sample_rate), nesting_level(0), pending_play_next(false),
    current(0), is_writing_message(false), non_idle_cnt(0)
{
  
}


MsgHandler::~MsgHandler(void)
{

} /* MsgHandler::~MsgHandler */


void MsgHandler::playFile(const string& path, bool idle_marked)
{
  QueueItem *item = 0;
  const char *ext = strrchr(path.c_str(), '.');
  if (strcmp(ext, ".gsm") == 0)
  {
    item = new GsmFileQueueItem(path, idle_marked);
  }
  else
  {
    item = new RawFileQueueItem(path, idle_marked);
  }
  addItemToQueue(item);
} /* MsgHandler::playFile */


void MsgHandler::playSilence(int length, bool idle_marked)
{
  QueueItem *item = new SilenceQueueItem(length, sample_rate, idle_marked);
  addItemToQueue(item);
} /* MsgHandler::playSilence */


void MsgHandler::playTone(int fq, int amp, int length, bool idle_marked)
{
  QueueItem *item = new ToneQueueItem(fq, amp, length, sample_rate,
      	      	      	      	      idle_marked);
  addItemToQueue(item);
} /* MsgHandler::playSilence */


void MsgHandler::clear(void)
{
  deleteQueueItem(current);
  current = 0;
  
  list<QueueItem*>::iterator it;
  for (it=msg_queue.begin(); it!=msg_queue.end(); ++it)
  {
    deleteQueueItem(*it);
  }
  
  non_idle_cnt = 0;
  
  msg_queue.clear();
  sinkFlushSamples();
} /* MsgHandler::clear */


void MsgHandler::begin(void)
{
  //printf("MsgHandler::begin: nesting_level=%d\n", nesting_level);
  if (nesting_level == 0)
  {
    pending_play_next = false;
  }
  ++nesting_level;
} /* MsgHandler::begin */


void MsgHandler::end(void)
{
  //printf("MsgHandler::end: nesting_level=%d\n", nesting_level);
  assert(nesting_level > 0);
  --nesting_level;
  if (nesting_level == 0)
  {
    if (pending_play_next)
    {
      pending_play_next = false;
      playMsg();
    }
    /*
    else
    {
      allMsgsWritten();
    }
    */
  }
} /* MsgHandler::end */


void MsgHandler::resumeOutput(void)
{
  if (current != 0)
  {
    writeSamples();
  }
} /* MsgHandler::resumeOutput */


void MsgHandler::deleteQueueItem(QueueItem *item)
{
  if (!item->idleMarked())
  {
    non_idle_cnt -= 1;
    assert(non_idle_cnt >= 0);
  }
  delete item;
} /* MsgHandler::deleteQueueItem */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


void MsgHandler::allSamplesFlushed(void)
{
  //printf("MsgHandler::allSamplesFlushed\n");
  is_writing_message = false;
  allMsgsWritten();
} /* MsgHandler::allSamplesFlushed */




/****************************************************************************
 *
 * Private member functions for class MsgHandler
 *
 ****************************************************************************/

void MsgHandler::addItemToQueue(QueueItem *item)
{
  is_writing_message = true;
  if (!item->idleMarked())
  {
    non_idle_cnt += 1;
  }
  msg_queue.push_back(item);
  if (msg_queue.size() == 1)
  {
    playMsg();
  }
} /* MsgHandler::addItemToQueue */


void MsgHandler::playMsg(void)
{
  if (nesting_level > 0)
  {
    pending_play_next = true;
    return;
  }
  
  if (msg_queue.empty())
  {
    sinkFlushSamples();
    return;
  }
  
  if (current != 0)
  {
    return;
  }
    
  current = msg_queue.front();
  msg_queue.pop_front();
  if (!current->initialize())
  {
    deleteQueueItem(current);
    playMsg();
  }
  else
  {
    writeSamples();
  }
} /* MsgHandler::playMsg */


void MsgHandler::writeSamples(void)
{
  float buf[WRITE_BLOCK_SIZE];

  assert(current != 0);
    
  int written;
  int read_cnt;
  do
  {
    read_cnt = current->readSamples(buf, sizeof(buf) / sizeof(*buf));
    if (read_cnt == 0)
    {
      goto done;
    }
    
    written = sinkWriteSamples(buf, read_cnt);
    if (written == -1)
    {
      perror("write in MsgHandler::writeFromFile");
      goto done;
    }
    //printf("Read=%d  Written=%d\n", read_cnt, written);
    
    if (written < read_cnt)
    {
      current->unreadSamples(read_cnt - written);
    }
  } while (written > 0);
  
  return;
    
  done:
    //printf("Done...\n");
    deleteQueueItem(current);
    current = 0;
    playMsg();
} /* MsgHandler::writeSamples */



/****************************************************************************
 *
 * Private member functions for class FileQueueItem
 *
 ****************************************************************************/

RawFileQueueItem::~RawFileQueueItem(void)
{
  if (file != -1)
  {
    ::close(file);
  }
} /* RawFileQueueItem::~FileQueueItem */


bool RawFileQueueItem::initialize(void)
{
  assert(file == -1);

  file = ::open(filename.c_str(), O_RDONLY);
  if (file == -1)
  {
    cerr << "*** WARNING: Could not find audio file \"" << filename << "\"\n";
    return false;
  }
    
  return true;
  
} /* RawFileQueueItem::initialize */


int RawFileQueueItem::readSamples(float *samples, int len)
{
  short buf[len];
  assert(file != -1);
  int read_cnt = read(file, buf, len * sizeof(*buf));
  if (read_cnt == -1)
  {
    perror("read in FileQueueItem::readSamples");
    read_cnt = 0;
  }
  else
  {
    read_cnt /= sizeof(*buf);
    for (int i=0; i<read_cnt; ++i)
    {
      samples[i] = static_cast<float>(buf[i]) / 32768.0;
    }
  }
  
  return read_cnt;
  
} /* RawFileQueueItem::readSamples */


void RawFileQueueItem::unreadSamples(int len)
{
  if (lseek(file, -len * sizeof(short), SEEK_CUR) == -1)
  {
    perror("lseek in RawFileQueueItem::unreadSamples");
  }
} /* RawFileQueueItem::unreadSamples */



/****************************************************************************
 *
 * Private member functions for class GsmFileQueueItem
 *
 ****************************************************************************/

GsmFileQueueItem::~GsmFileQueueItem(void)
{
  if (decoder != 0)
  {
    gsm_destroy(decoder);
  }
  
  if (file != -1)
  {
    ::close(file);
  }
} /* GsmFileQueueItem::~FileQueueItem */


bool GsmFileQueueItem::initialize(void)
{
  //cout << "GsmFileQueueItem::initialize\n";
  
  assert(file == -1);

  file = ::open(filename.c_str(), O_RDONLY);
  if (file == -1)
  {
    cerr << "*** WARNING: Could not find audio file \"" << filename << "\"\n";
    return false;
  }
  
  decoder = gsm_create();
  
  buf_pos = BUFSIZE;
  
  return true;
  
} /* GsmFileQueueItem::initialize */


int GsmFileQueueItem::readSamples(float *samples, int len)
{
  int read_cnt = 0;
  
  //cout << "buf_pos=" << buf_pos << endl;
  if (buf_pos == BUFSIZE)
  {
    assert(file != -1);

    gsm_frame gsm_data;
    int cnt = read(file, gsm_data, sizeof(gsm_data));
    if (cnt == -1)
    {
      perror("read in GsmFileQueueItem::readSamples");
      return 0;
    }
    else if (cnt != sizeof(gsm_data))
    {
      if (cnt != 0)
      {
      	cerr << "*** WARNING: Corrupt GSM file: " << filename << endl;
      }
      
      return 0;
    }
    else
    {
      gsm_decode(decoder, gsm_data, buf);
      buf_pos = 0;
    }
  }
  
  while ((read_cnt < len) && (buf_pos < BUFSIZE))
  {
    samples[read_cnt++] = static_cast<float>(buf[buf_pos++]) / 32768.0;
  }
  
  //cout << "GsmFileQueueItem::readSamples: " << read_cnt << endl;
  
  return read_cnt;
  
} /* GsmFileQueueItem::readSamples */


void GsmFileQueueItem::unreadSamples(int len)
{
  //cout << "GsmFileQueueItem::unreadSamples(" << len << ")\n";
  if (len > buf_pos)
  {
    cerr << "*** WARNING: Trying to unread more GSM samples then was "
            "previously read." << endl;
    return;
  }
  
  buf_pos -= len;
  
  //cout << "GsmFileQueueItem::unreadSamples: buf_pos=" << buf_pos << endl;
  
} /* GsmFileQueueItem::unreadSamples */



/****************************************************************************
 *
 * Private member functions for class SilenceQueueItem
 *
 ****************************************************************************/

int SilenceQueueItem::readSamples(float *samples, int len)
{
  assert(silence_left != -1);
  
  int read_cnt = min(len, silence_left);
  memset(samples, 0, sizeof(*samples) * read_cnt);
  if (silence_left == 0)
  {
    silence_left = -1;
  }
  else
  {
    silence_left -= read_cnt;
  }
  
  return read_cnt;
  
} /* SilenceQueueItem::readSamples */


void SilenceQueueItem::unreadSamples(int len)
{
  silence_left += len;
} /* SilenceQueueItem::unreadSamples */




/****************************************************************************
 *
 * Private member functions for class ToneQueueItem
 *
 ****************************************************************************/

int ToneQueueItem::readSamples(float *samples, int len)
{
  int read_cnt = min(len, tone_len-pos);
  for (int i=0; i<read_cnt; ++i)
  {
    samples[i] = amp / 1000.0 * sin(2 * M_PI * fq * pos / sample_rate);
    ++pos;
  }
  
  return read_cnt;
  
} /* ToneQueueItem::readSamples */


void ToneQueueItem::unreadSamples(int len)
{
  pos -= len;
} /* ToneQueueItem::unreadSamples */




/*
 * This file has not been truncated
 */
