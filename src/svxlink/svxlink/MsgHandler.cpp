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
#include <stdint.h>

extern "C" {
#include <gsm.h>
}

#include <cassert>
#include <iostream>
#include <cstring>



/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFileReader.h>

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
      : QueueItem(idle_marked), filename(filename), reader(16384) {}
    bool initialize(void);
    int readSamples(float *samples, int len);

  private:
    string  filename;
    Async::FileReader reader;
    
};

class GsmFileQueueItem : public QueueItem
{
  public:
    GsmFileQueueItem(const std::string& filename, bool idle_marked)
      : QueueItem(idle_marked), filename(filename), reader(4096), decoder(0) {}
    ~GsmFileQueueItem(void);
    bool initialize(void);
    int readSamples(float *samples, int len);

  private:
    static const int GSM_BUFSIZE = 160;
    
    string    	filename;
    Async::FileReader reader;
    gsm       	decoder;
    int       	buf_pos;
    int         buf_avail;
    gsm_signal  buf[GSM_BUFSIZE];

};

class WavFileQueueItem : public QueueItem
{
  public:
    WavFileQueueItem(const std::string& filename, bool idle_marked)
      : QueueItem(idle_marked), filename(filename), reader(16384) {}
    bool initialize(void);
    int readSamples(float *samples, int len);

  private:
    static const int WAV_BUFSIZE = 512;
  
    string    filename;
    Async::FileReader reader;
    char      chunk_id[4];
    uint32_t  chunk_size;
    char      format[4];
    char      subchunk1id[4];
    uint32_t  subchunk1size;
    uint16_t  audio_format;
    uint16_t  num_channels;
    uint32_t  sample_rate;
    uint32_t  byte_rate;
    uint16_t  block_align;
    uint16_t  bits_per_sample;
    char      subchunk2id[4];
    uint32_t  subchunk2size;
    
    int read32bitValue(unsigned char *ptr, uint32_t *val);
    int read16bitValue(unsigned char *ptr, uint16_t *val);
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
  else if (strcmp(ext, ".wav") == 0)
  {
    item = new WavFileQueueItem(path, idle_marked);
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


void MsgHandler::requestSamples(int count)
{
  if (current != 0)
  {
    writeSamples(count);
  }
} /* MsgHandler::requestSamples */


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
    current = 0;
    playMsg();
  }
  else
  {
    sinkAvailSamples();
  }
} /* MsgHandler::playMsg */


void MsgHandler::writeSamples(int count)
{
  float buf[count];

  int read_cnt = current->readSamples(buf, count);
  if (read_cnt == 0)
  {
    deleteQueueItem(current);
    current = 0;
    playMsg();
    return;
  }
    
  sinkWriteSamples(buf, read_cnt);

} /* MsgHandler::writeSamples */



/****************************************************************************
 *
 * Private member functions for class FileQueueItem
 *
 ****************************************************************************/

bool RawFileQueueItem::initialize(void)
{
  if (!reader.open(filename))
  {
    cerr << "*** WARNING: Could not find audio file \"" << filename << "\"\n";
    return false;
  }
  
  return true;
  
} /* RawFileQueueItem::initialize */


int RawFileQueueItem::readSamples(float *samples, int len)
{
  short buf[len];
  int cnt = reader.read(buf, len * sizeof(short));
  if (cnt < 0)
  {
    perror("read in FileQueueItem::readSamples");
    return 0;
  }

  cnt /= sizeof(short);
  for (int i = 0; i < cnt; i++)
  {
    samples[i] = static_cast<float>(buf[i]) / 32768.0;
  }
          
  return cnt;
  
} /* RawFileQueueItem::readSamples */


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
} /* GsmFileQueueItem::~FileQueueItem */


bool GsmFileQueueItem::initialize(void)
{
  //cout << "GsmFileQueueItem::initialize\n";
  
  if (!reader.open(filename))
  {
    cerr << "*** WARNING: Could not find audio file \"" << filename << "\"\n";
    return false;
  }
  
  decoder = gsm_create();
  
  buf_pos = buf_avail = 0;
  
  return true;
  
} /* GsmFileQueueItem::initialize */


int GsmFileQueueItem::readSamples(float *samples, int len)
{
  int read_cnt = 0;
  
  //cout << "buf_pos=" << buf_pos << endl;
  if (buf_pos == buf_avail)
  {
    gsm_frame gsm_data;
    int cnt = reader.read(gsm_data, sizeof(gsm_data));
    if (cnt < 0)
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
      buf_avail = GSM_BUFSIZE;
    }
  }
  
  while ((read_cnt < len) && (buf_pos < buf_avail))
  {
    samples[read_cnt++] = static_cast<float>(buf[buf_pos++]) / 32768.0;
  }
  
  //cout << "GsmFileQueueItem::readSamples: " << read_cnt << endl;
  
  return read_cnt;
  
} /* GsmFileQueueItem::readSamples */


/****************************************************************************
 *
 * Private member functions for class WavFileQueueItem
 *
 ****************************************************************************/

bool WavFileQueueItem::initialize(void)
{
  if (!reader.open(filename))
  {
    cerr << "*** WARNING: Could not find audio file \"" << filename << "\"\n";
    return false;
  }
  
    // Read the wave file header
  unsigned char buf[44];
  int read_cnt = reader.read(buf, sizeof(buf));
  if (read_cnt != sizeof(buf))
  {
    cerr << "*** WARNING: Illegal WAV file header. Header too short: "
      	 << filename << "\n";
    return false;
  }
  
  unsigned char *ptr = buf;
  memcpy(chunk_id, ptr, 4);
  ptr += 4;
  if (memcmp(chunk_id, "RIFF", 4) != 0)
  {
    cerr << "*** WARNING: Illegal WAV file header. ChunkID is not \"RIFF\": "
      	 << filename << "\n";
    return false;
  }
  
  ptr += read32bitValue(ptr, &chunk_size);
  
  memcpy(format, ptr, 4);
  ptr += 4;
  if (memcmp(format, "WAVE", 4) != 0)
  {
    cerr << "*** WARNING: Illegal WAV file header. Format is not \"WAVE\": "
      	 << filename << "\n";
    return false;
  }
  
  memcpy(subchunk1id, ptr, 4);
  ptr += 4;
  if (memcmp(subchunk1id, "fmt ", 4) != 0)
  {
    cerr << "*** WARNING: Illegal WAV file header. "
      	 << "Subchunk1ID is not \"fmt \": "
      	 << filename << "\n";
    return false;
  }
  
  ptr += read32bitValue(ptr, &subchunk1size);
  if (subchunk1size != 16)
  {
    cerr << "*** WARNING: Illegal WAV file header. "
      	 << "Subchunk1Size is not 16: "
      	 << filename << "\n";
    return false;
  }
  
  ptr += read16bitValue(ptr, &audio_format);
  if (audio_format != 1)
  {
    cerr << "*** WARNING: SvxLink can only handle PCM formated WAV files: "
      	 << filename << "\n";
    return false;
  }
  
  ptr += read16bitValue(ptr, &num_channels);
  if (num_channels != 1)
  {
    cerr << "*** WARNING: SvxLink can only handle mono WAV files: "
      	 << filename << "\n";
    return false;
  }
  
  ptr += read32bitValue(ptr, &sample_rate);
  if (sample_rate != INTERNAL_SAMPLE_RATE)
  {
    cerr << "*** WARNING: SvxLink can only handle WAV files with sample rate "
      	 << INTERNAL_SAMPLE_RATE << ": "
      	 << filename << "\n";
    return false;
  }
  
  ptr += read32bitValue(ptr, &byte_rate);
  
  ptr += read16bitValue(ptr, &block_align);
  
  ptr += read16bitValue(ptr, &bits_per_sample);
  if (bits_per_sample != 16)
  {
    cerr << "*** WARNING: SvxLink can only handle WAV files with "
      	 << "16 bits per sample: "
      	 << filename << "\n";
    return false;
  }
  
  memcpy(subchunk2id, ptr, 4);
  ptr += 4;
  if (memcmp(subchunk2id, "data", 4) != 0)
  {
    cerr << "*** WARNING: Illegal WAV file header. "
      	 << "Subchunk2ID is not \"data\": "
      	 << filename << "\n";
    return false;
  }
  
  ptr += read32bitValue(ptr, &subchunk2size);
  if (chunk_size != subchunk2size + 36)
  {
    cerr << "*** WARNING: Illegal WAV file header. "
      	 << "ChunkSize should be Subchunk2Size plus 36: "
      	 << filename << "\n";
    return false;
  }

  return true;
  
} /* WavFileQueueItem::initialize */


int WavFileQueueItem::readSamples(float *samples, int len)
{
  short buf[len];
  int cnt = reader.read(buf, len * sizeof(short));
  if (cnt < 0)
  {
    perror("read in FileQueueItem::readSamples");
    return 0;
  }

  cnt /= sizeof(short);
  for (int i = 0; i < cnt; i++)
  {
    samples[i] = static_cast<float>(buf[i]) / 32768.0;
  }
          
  return cnt;
  
} /* WavFileQueueItem::readSamples */


int WavFileQueueItem::read32bitValue(unsigned char *ptr, uint32_t *val)
{
  *val = ptr[0] + (ptr[1] << 8) + (ptr[2] << 16) + (ptr[3] << 24);
  return 4;
} /* WavFileQueueItem::read32bitValue */


int WavFileQueueItem::read16bitValue(unsigned char *ptr, uint16_t *val)
{
  *val = ptr[0] + (ptr[1] << 8);
  return 2;
} /* WavFileQueueItem::read16bitValue */



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



/*
 * This file has not been truncated
 */
