/**
@file	 MsgHandler.cpp
@brief   Handling of playback of audio clips
@author  Tobias Blomberg / SM0SVX
@date	 2005-10-22

This file contains an object that handles the playback of audio clips.

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
#include <fstream>
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

//#define WRITE_BLOCK_SIZE    4*160
#define WRITE_BLOCK_SIZE    256



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

class DtmfQueueItem : public QueueItem
{
  public:
    DtmfQueueItem(int fqh, int fql, int amp, int len, int sample_rate,
                  bool idle_marked)
      : QueueItem(idle_marked), fqh(fqh), fql(fql), amp(amp),
        tone_len(sample_rate * len / 1000), pos(0), sample_rate(sample_rate) {}
    int readSamples(float *samples, int len);
    void unreadSamples(int len);

  private:
    int fqh;
    int fql;
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
      : QueueItem(idle_marked), filename(filename), file(-1), decoder(0),
        buf_pos(0) {}
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

class WavFileQueueItem : public QueueItem
{
  public:
    WavFileQueueItem(const std::string& filename, bool idle_marked)
      : QueueItem(idle_marked), filename(filename), chunk_size(0),
        subchunk1size(0), audio_format(0), num_channels(0), sample_rate(0),
        byte_rate(0), block_align(0), bits_per_sample(0), subchunk2size(0),
        data_read(0)
    {}
    ~WavFileQueueItem(void);
    bool initialize(void);
    int readSamples(float *samples, int len);
    void unreadSamples(int len);

  private:
    string    filename;
    ifstream  file;
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
    uint32_t  data_read;
    
    int read32bitValue(uint8_t *ptr, uint32_t *val);
    int read16bitValue(uint8_t *ptr, uint16_t *val);
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
  clearP();
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
} /* MsgHandler::playTone */


void MsgHandler::playDtmf(char digit, int amp, int length, bool idle_marked)
{
  static map<char, pair<int, int> > tone_map;
  tone_map['1'] = pair<int, int>(697, 1209);
  tone_map['2'] = pair<int, int>(697, 1336);
  tone_map['3'] = pair<int, int>(697, 1477);
  tone_map['A'] = pair<int, int>(697, 1633);
  tone_map['4'] = pair<int, int>(770, 1209);
  tone_map['5'] = pair<int, int>(770, 1336);
  tone_map['6'] = pair<int, int>(770, 1477);
  tone_map['B'] = pair<int, int>(770, 1633);
  tone_map['7'] = pair<int, int>(852, 1209);
  tone_map['8'] = pair<int, int>(852, 1336);
  tone_map['9'] = pair<int, int>(852, 1477);
  tone_map['C'] = pair<int, int>(852, 1633);
  tone_map['*'] = pair<int, int>(941, 1209);
  tone_map['0'] = pair<int, int>(941, 1336);
  tone_map['#'] = pair<int, int>(941, 1477);
  tone_map['D'] = pair<int, int>(941, 1633);

  int fql = tone_map[digit].first;
  int fqh = tone_map[digit].second;

  if (fql > 0)
  {
    QueueItem *item = new DtmfQueueItem(fqh, fql, amp, length, sample_rate,
                                        idle_marked);
    addItemToQueue(item);
  }
} /* MsgHandler::playDtmf */


void MsgHandler::clear(void)
{
  clearP();
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


void MsgHandler::deleteQueueItem(QueueItem *item)
{
  if (item == 0)
  {
    return;
  }
  if (!item->idleMarked())
  {
    non_idle_cnt -= 1;
    assert(non_idle_cnt >= 0);
  }
  delete item;
} /* MsgHandler::deleteQueueItem */


void MsgHandler::clearP(void)
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
} /* MsgHandler::clearP */



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
 * Private member functions for class WavFileQueueItem
 *
 ****************************************************************************/

WavFileQueueItem::~WavFileQueueItem(void)
{
  if (file.is_open())
  {
    file.close();
  }
} /* WavFileQueueItem::~WavFileQueueItem */


bool WavFileQueueItem::initialize(void)
{
  assert(!file.is_open());

  file.open(filename.c_str(), ios::in | ios::binary);
  if (!file.is_open())
  {
    cerr << "*** WARNING: Could not read WAV file \"" << filename << "\"\n";
    return false;
  }
  
    // Read the RIFF chunk descriptor
  char chunk_header[12];
  file.read(chunk_header, sizeof(chunk_header));
  if (!file.good())
  {
    cerr << "*** WARNING: Failed to read WAV file \"" << filename << "\": "
         << strerror(errno) << endl;
    return false;
  }
  
  uint8_t *ptr = reinterpret_cast<uint8_t*>(chunk_header);
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
  if (memcmp(format, "WAVE", 4) != 0)
  {
    cerr << "*** WARNING: Illegal WAV file header. Format is not \"WAVE\": "
      	 << filename << "\n";
    return false;
  }

    // Read subchunks
  std::streampos data_file_pos(0);
  int64_t rest_size = chunk_size - 4;
  while (rest_size > 0)
  {
      // Read subchunk header
    char subchunk_header[8];
    file.read(subchunk_header, sizeof(subchunk_header));
    if (file.fail())
    {
      cerr << "*** WARNING: Failed to read WAV file \""
           << filename << "\" subchunk header: "
           << strerror(errno) << endl;
      return false;
    }

    ptr = reinterpret_cast<uint8_t*>(subchunk_header);

    char subchunk_id[5];
    memcpy(subchunk_id, ptr, 4);
    subchunk_id[4] = 0;
    ptr += 4;

    uint32_t subchunk_size;
    ptr += read32bitValue(ptr, &subchunk_size);

    rest_size -= subchunk_size + 8;

    if (memcmp(subchunk_id, "data", 4) == 0)
    {
      memcpy(subchunk2id, subchunk_id, 4);
      subchunk2size = subchunk_size;

      data_file_pos = file.tellg();
      file.seekg(subchunk_size, ios::cur);
      if (!file.good())
      {
        cerr << "*** WARNING: Could not seek to end of subchunk data in "
                "WAV file \"" << filename << "\": "
             << strerror(errno) << endl;
        return false;
      }
    }
    else
    {
      if (memcmp(subchunk_id, "fmt ", 4) == 0)
      {
        if (subchunk_size != 16)
        {
          cerr << "*** WARNING: Illegal WAV file header in \""
               << filename << "\". "
               << "\"fmt\" subchunk size is not 16: "
               << filename << "\n";
          return false;
        }
        memcpy(subchunk1id, subchunk_id, 4);
        subchunk1size = subchunk_size;

          // Read subchunk data
        char subchunk_data[subchunk_size];
        file.read(subchunk_data, subchunk_size);
        if (!file.good())
        {
          cerr << "*** WARNING: Failed to read WAV file \""
               << filename << "\"subchunk data: "
               << strerror(errno) << endl;
          return false;
        }
        ptr = reinterpret_cast<uint8_t*>(subchunk_data);

        ptr += read16bitValue(ptr, &audio_format);
        if (audio_format != 1)
        {
          cerr << "*** WARNING: SvxLink can only handle PCM formatted WAV files: "
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
          cerr << "*** WARNING: SvxLink can only handle WAV files which have "
                  "a sample rate of "
               << INTERNAL_SAMPLE_RATE << ". This file use " << sample_rate
               << ": " << filename << "\n";
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
      }
      else
      {
          // Skip unknown subchunk data
        file.seekg(subchunk_size, ios::cur);
        if (!file.good())
        {
          cerr << "*** WARNING: Could not seek to end of subchunk data in "
                  "WAV file \"" << filename << "\": "
               << strerror(errno) << endl;
          return false;
        }
      }
    }
  }

  if (rest_size != 0)
  {
    cerr << "*** WARNING: Illegal WAV file header. "
         << "ChunkSize should be the sum of all subchunk sizes (rest_size="
         << rest_size << "): "
      	 << filename << "\n";
    return false;
  }

  if (subchunk1size == 0)
  {
    cerr << "*** WARNING: No \"fmt\" subchunk found in WAV file \""
         << filename << "\"" << endl;
    return false;
  }

  if (data_file_pos == std::streampos(0))
  {
    cerr << "*** WARNING: No \"data\" subchunk found in WAV file \""
         << filename << "\"" << endl;
    return false;
  }

    // Set file position to the start of audio data
  file.seekg(data_file_pos, ios::beg);
  if (!file.good())
  {
    cerr << "*** WARNING: Failed to seek to audio data position in file \""
         << filename << "\":" << strerror(errno) << endl;
    return false;
  }
  
  return true;
  
} /* WavFileQueueItem::initialize */


int WavFileQueueItem::readSamples(float *samples, int len)
{
  short buf[len];
  assert(file.is_open());
  streamsize to_read = min(static_cast<uint32_t>(len * sizeof(*buf)),
                           subchunk2size - data_read);
  if (to_read <= 0)
  {
    return 0;
  }

  file.read(reinterpret_cast<char*>(buf), to_read);
  if (file.fail())
  {
    cerr << "*** WARNING: Failed to read samples from WAV file \""
         << filename << "\": " << strerror(errno) << endl;
    return 0;
  }

  int read_cnt = file.gcount();
  data_read += read_cnt;
  read_cnt /= sizeof(*buf);
  for (int i=0; i<read_cnt; ++i)
  {
    samples[i] = static_cast<float>(buf[i]) / 32768.0;
  }
  
  return read_cnt;
} /* WavFileQueueItem::readSamples */


void WavFileQueueItem::unreadSamples(int len)
{
  assert(len >= 0);
  streamsize to_unread = min(static_cast<uint32_t>(len * sizeof(short)),
                             data_read);
  if (!file.seekg(-to_unread, ios::cur).good())
  {
    cerr << "***ERROR: Failed to unread samples from WAV file \""
         << filename << "\": " << strerror(errno) << endl;
  }
  data_read -= to_unread;
} /* WavFileQueueItem::unreadSamples */


int WavFileQueueItem::read32bitValue(uint8_t *ptr, uint32_t *val)
{
  *val = ptr[0] + (ptr[1] << 8) + (ptr[2] << 16) + (ptr[3] << 24);
  return 4;
} /* WavFileQueueItem::read32bitValue */


int WavFileQueueItem::read16bitValue(uint8_t *ptr, uint16_t *val)
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



/****************************************************************************
 *
 * Private member functions for class DtmfQueueItem
 *
 ****************************************************************************/

int DtmfQueueItem::readSamples(float *samples, int len)
{
  int read_cnt = min(len, tone_len-pos);
  for (int i=0; i<read_cnt; ++i)
  {
    samples[i] = amp / 1000.0 * (
                sin(2 * M_PI * fqh * pos / sample_rate) +
                sin(2 * M_PI * fql * pos / sample_rate));
    ++pos;
  }

  return read_cnt;
} /* DtmfQueueItem::readSamples */


void DtmfQueueItem::unreadSamples(int len)
{
  pos -= len;
} /* DtmfQueueItem::unreadSamples */



/*
 * This file has not been truncated
 */
