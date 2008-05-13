#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "MsgHandler.h"


#define WRITE_BLOCK_SIZE    4*160


using namespace std;


MsgHandler::MsgHandler(const string& base_dir, int sample_rate)
  : file(-1), base_dir(base_dir), silence_left(-1), sample_rate(sample_rate),
    play_next(true), pending_play_next(false)
{
  
}


MsgHandler::~MsgHandler(void)
{

} /* MsgHandler::~MsgHandler */


void MsgHandler::playFile(const string& path)
{
  char *str = new char[path.length() + 7];
  sprintf(str, ">FILE:%s", path.c_str());
  playMsg("Default", str);
  delete [] str;
} /* MsgHandler::playFile */


void MsgHandler::playMsg(const string& context, const string& msg)
{
  msg_queue.push_back(MsgQueueItem(context, msg));
  if (msg_queue.size() == 1)
  {
    playNextMsg();
  }
}


void MsgHandler::playNumber(float number)
{
  int intpart = static_cast<int>(number);
  number -= intpart;
  number *= 10;
  //int fracpart = static_cast<int>((number-intpart)*1000);
  
  list<string> digits;

  if (number > 0)
  {
    do
    {
      char digit[2];
      digit[0] = static_cast<char>(number) + '0';
      digit[1] = 0;
      number = (number - static_cast<int>(number)) * 10;
      digits.push_back(digit);
    } while (number > 0);
    digits.push_front("decimal");
  }
  
  do
  {
    char digit[2];
    digit[0] = (intpart % 10) + '0';
    digit[1] = 0;
    intpart /= 10;
    digits.push_front(digit);
  } while (intpart != 0);
  
  begin();
  list<string>::iterator it;
  for (it=digits.begin(); it!=digits.end(); ++it)
  {
    playMsg("Default", *it);
  }
  end();
}


void MsgHandler::spellWord(const string& word)
{
  for (unsigned i=0; i<word.size(); ++i)
  {
    char ch[] = "phonetic_?";
    ch[9] = tolower(word[i]);
    playMsg("Default", ch);
  }
}


void MsgHandler::playSilence(int length)
{
  char str[256];
  sprintf(str, ">SILENCE:%d", length);
  playMsg("Default", str);
} /* MsgHandler::playSilence */


void MsgHandler::clear(void)
{
  msg_queue.clear();
  ::close(file);
  file = -1;
  sinkFlushSamples();
  allMsgsWritten();
} /* MsgHandler::clear */


void MsgHandler::begin(void)
{
  pending_play_next = false;
  play_next = false;
} /* MsgHandler::begin */


void MsgHandler::end(void)
{
  play_next = true;
  if (pending_play_next)
  {
    pending_play_next = false;
    playNextMsg();
  }
} /* MsgHandler::end */


void MsgHandler::resumeOutput(void)
{
  if (!msg_queue.empty())
  {
    writeFromFile();
  }
} /* MsgHandler::resumeOutput */





void MsgHandler::playNextMsg(void)
{
  if (!play_next)
  {
    pending_play_next = true;
    return;
  }
  
  if (msg_queue.empty())
  {
    sinkFlushSamples();
    allMsgsWritten();
    return;
  }
    
  MsgQueueItem& msg = msg_queue.front();
  
  if (msg.msg[0] == '>')
  {
    executeCmd(msg.msg);
  }
  else
  {
    file = ::open((base_dir + "/" + msg.context + "/" + msg.msg + ".raw").c_str(),
      	      	  O_RDONLY);
    if (file == -1)
    {
      file = ::open((base_dir + "/" + "Default/" + msg.msg + ".raw").c_str(),
      	      	    O_RDONLY);
    }
    if (file != -1)
    {
      writeFromFile();
    }
    else
    {
      cerr << "*** WARNING: Could not find audio message \"" << msg.msg
           << "\"\n";
      msg_queue.pop_front();
      playNextMsg();
    }
  }
}


void MsgHandler::executeCmd(const string& cmd)
{
  if (strstr(cmd.c_str(), ">SILENCE:") == cmd.c_str())
  {
    silence_left = sample_rate * atoi(cmd.c_str() + 9) / 1000;
    writeFromFile();
  }
  else if (strstr(cmd.c_str(), ">FILE:") == cmd.c_str())
  {
    file = ::open(cmd.c_str() + 6, O_RDONLY);
    if (file != -1)
    {
      writeFromFile();
    }
    else
    {
      cerr << "*** WARNING: Could not find audio file \"" << cmd.c_str() + 6
           << "\"\n";
      msg_queue.pop_front();
      playNextMsg();
    }
  }
  else
  {
    cerr << "*** WARNING: Unknown audio command: \"" << cmd << "\"\n";
    msg_queue.pop_front();
    playNextMsg();
  }
  
} /* MsgHandler::executeCmd */


void MsgHandler::writeFromFile(void)
{
  float samples[WRITE_BLOCK_SIZE];
  
  int written;
  int read_cnt;
  do
  {
    read_cnt = readSamples(samples, WRITE_BLOCK_SIZE);
    if (read_cnt == 0)
    {
      goto done;
    }
    
    written = sinkWriteSamples(samples, read_cnt);
    if (written == -1)
    {
      perror("write in MsgHandler::writeFromFile");
      goto done;
    }
    //printf("Read=%d  Written=%d\n", read_cnt, written);
  } while (written == read_cnt);
  
  unreadSamples(read_cnt - written);
  
  return;
    
  done:
    //printf("Done...\n");
    if (file != -1)
    {
      ::close(file);
      file = -1;
    }
    
    msg_queue.pop_front();
    playNextMsg();
} /* MsgHandler::writeFromFile */


int MsgHandler::readSamples(float *samples, int len)
{
  int read_cnt;
  int16_t buf[len];
  
  if (silence_left >= 0)
  {
    read_cnt = min(len, silence_left);
    memset(samples, 0, sizeof(*samples) * read_cnt);
    if (silence_left == 0)
    {
      silence_left = -1;
    }
    else
    {
      silence_left -= read_cnt;
    }
    //printf("Reading %d silence samples\n", read_cnt);
  }
  else
  {
    read_cnt = read(file, buf, len * sizeof(*buf));
    if (read_cnt == -1)
    {
      perror("read in MsgHandler::readSamples");
      read_cnt = 0;
    }
    else
    {
      read_cnt /= sizeof(*buf);
      for (int i=0; i<read_cnt; ++i)
      {
      	samples[i] = static_cast<float>(buf[i]) / 32768;
      }
    }
  }
  
  return read_cnt;
  
} /* MsgHandler::readSamples */


void MsgHandler::unreadSamples(int len)
{
  if (silence_left >= 0)
  {
    silence_left += len;
  }
  else
  {
    //printf("lseeking...\n");
    lseek(file, -len * sizeof(int16_t), SEEK_CUR);
  }
    

} /* MsgHandler::unreadSamples */

