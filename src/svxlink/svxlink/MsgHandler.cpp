#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include <cassert>
#include <iostream>

#include "MsgHandler.h"


#define WRITE_BLOCK_SIZE    4*160


using namespace std;


class QueueItem
{
  public:
    virtual ~QueueItem(void) {}
    virtual bool initialize(void) { return true; }
    virtual int readSamples(short *samples, int len) = 0;
    virtual void unreadSamples(int len) = 0;
};

class SilenceQueueItem : public QueueItem
{
  public:
    SilenceQueueItem(int len, int sample_rate)
      : len(len), silence_left(sample_rate * len / 1000) {}
    int readSamples(short *samples, int len);
    void unreadSamples(int len);

  private:
    int len;
    int silence_left;
    
};

class ToneQueueItem : public QueueItem
{
  public:
    ToneQueueItem(int fq, int amp, int len, int sample_rate)
      : fq(fq), amp(amp), tone_len(sample_rate * len / 1000), pos(0),
      	sample_rate(sample_rate) {}
    int readSamples(short *samples, int len);
    void unreadSamples(int len);

  private:
    int fq;
    int amp;
    int tone_len;
    int pos;
    int sample_rate;
    
};

class FileQueueItem : public QueueItem
{
  public:
    FileQueueItem(const std::string& filename) : filename(filename), file(-1) {}
    ~FileQueueItem(void);
    bool initialize(void);
    int readSamples(short *samples, int len);
    void unreadSamples(int len);

  private:
    string  filename;
    int     file;
    
};




using namespace std;


MsgHandler::MsgHandler(int sample_rate)
  : sample_rate(sample_rate), nesting_level(0), pending_play_next(false)
{
  
}


MsgHandler::~MsgHandler(void)
{

} /* MsgHandler::~MsgHandler */


void MsgHandler::playFile(const string& path)
{
  QueueItem *item = new FileQueueItem(path);
  addItemToQueue(item);
} /* MsgHandler::playFile */


void MsgHandler::playSilence(int length)
{
  QueueItem *item = new SilenceQueueItem(length, sample_rate);
  addItemToQueue(item);
} /* MsgHandler::playSilence */


void MsgHandler::playTone(int fq, int amp, int length)
{
  QueueItem *item = new ToneQueueItem(fq, amp, length, sample_rate);
  addItemToQueue(item);
} /* MsgHandler::playSilence */


void MsgHandler::writeBufferFull(bool is_full)
{
  //cout << "MsgHandler::writeBufferFull: write_buffer_full=" << is_full
  //     << "  msg_queue.empty()=" << msg_queue.empty() << endl;
  if (!is_full && !msg_queue.empty())
  {
    playMsg();
  }
}


void MsgHandler::clear(void)
{
  list<QueueItem*>::iterator it;
  for (it=msg_queue.begin(); it!=msg_queue.end(); ++it)
  {
    delete *it;
  }
  msg_queue.clear();
  allMsgsWritten();
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
    else
    {
      allMsgsWritten();
    }
  }
} /* MsgHandler::end */










void MsgHandler::addItemToQueue(QueueItem *item)
{
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
    allMsgsWritten();
    return;
  }
    
  QueueItem *item = msg_queue.front();
  if (!item->initialize())
  {
    msg_queue.pop_front();
    delete item;
    playMsg();
  }
  else
  {
    writeSamples();
  }
}


void MsgHandler::writeSamples(void)
{
  short buf[WRITE_BLOCK_SIZE];
  
  QueueItem *item = msg_queue.front();
  
  int written;
  int read_cnt;
  do
  {
    read_cnt = item->readSamples(buf, sizeof(buf) / sizeof(buf[0]));
    if (read_cnt == 0)
    {
      goto done;
    }
    
    written = writeAudio(buf, read_cnt);
    if (written == -1)
    {
      perror("write in MsgHandler::writeFromFile");
      goto done;
    }
    //printf("Read=%d  Written=%d\n", read_cnt, written);
  } while (written == read_cnt);
  
  item->unreadSamples(read_cnt - written);
  
  return;
    
  done:
    //printf("Done...\n");
    msg_queue.pop_front();
    delete item;
    playMsg();
}




FileQueueItem::~FileQueueItem(void)
{
  if (file != -1)
  {
    ::close(file);
  }
} /* FileQueueItem::~FileQueueItem */


bool FileQueueItem::initialize(void)
{
  if (file == -1)
  {
    file = ::open(filename.c_str(), O_RDONLY);
    if (file == -1)
    {
      cerr << "*** WARNING: Could not find audio file \"" << filename << "\"\n";
      return false;
    }
  }
    
  return true;
  
} /* FileQueueItem::initialize */


int FileQueueItem::readSamples(short *samples, int len)
{
  assert(file != -1);
  int read_cnt = read(file, samples, len * sizeof(*samples));
  if (read_cnt == -1)
  {
    perror("read in FileQueueItem::readSamples");
    read_cnt = 0;
  }
  else
  {
    read_cnt /= sizeof(*samples);
  }
  
  return read_cnt;
  
} /* FileQueueItem::readSamples */


void FileQueueItem::unreadSamples(int len)
{
  if (lseek(file, -len * sizeof(short), SEEK_CUR) == -1)
  {
    perror("lseek in FileQueueItem::unreadSamples");
  }
} /* FileQueueItem::unreadSamples */




int SilenceQueueItem::readSamples(short *samples, int len)
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





int ToneQueueItem::readSamples(short *samples, int len)
{
  int read_cnt = min(len, tone_len-pos);
  for (int i=0; i<read_cnt; ++i)
  {
    samples[i] = static_cast<short>(
      	    amp / 1000.0 * 32767.0 * sin(2*M_PI*fq*pos/sample_rate));
    ++pos;
  }
  
  return read_cnt;
  
} /* ToneQueueItem::readSamples */


void ToneQueueItem::unreadSamples(int len)
{
  pos -= len;
} /* ToneQueueItem::unreadSamples */



