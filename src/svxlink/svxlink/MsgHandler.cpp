#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>


#include "MsgHandler.h"


using namespace std;


MsgHandler::MsgHandler(const string& base_dir)
  : file(-1), base_dir(base_dir)
{
  
}


void MsgHandler::playMsg(const string& context, const string& msg)
{
  msg_queue.push_back(MsgQueueItem(context, msg));
  if (msg_queue.size() == 1)
  {
    playNextMsg();
  }
}


void MsgHandler::playNumber(int number)
{
  list<string> digits;
  do
  {
    char digit[2];
    digit[0] = (number % 10) + '0';
    digit[1] = 0;
    number /= 10;
    digits.push_front(digit);
  } while (number != 0);
  
  list<string>::iterator it;
  for (it=digits.begin(); it!=digits.end(); ++it)
  {
    playMsg("Default", *it);
  }
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


void MsgHandler::writeBufferFull(bool is_full)
{
  //printf("write_buffer_full=%s\n", is_full ? "true" : "false");
  if (!is_full && !msg_queue.empty())
  {
    writeFromFile();
  }
}


void MsgHandler::clear(void)
{
  msg_queue.clear();
  ::close(file);
  file = -1;
  allMsgsWritten();
} /* MsgHandler::clear */





void MsgHandler::playNextMsg(void)
{
  if (msg_queue.size() == 0)
  {
    return;
  }
    
  MsgQueueItem& msg = msg_queue.front();
  
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
    msg_queue.pop_front();
    if (msg_queue.empty())
    {
      allMsgsWritten();
    }
    else
    {
      playNextMsg();
    }
  }  
}


void MsgHandler::writeFromFile(void)
{
  short buf[160];
  assert(file != -1);
  bool success = false;
  
  int written;
  int read_cnt;
  do
  {
    read_cnt = read(file, buf, sizeof(buf));
    if (read_cnt == -1)
    {
      perror("read in MsgHandler::writeFromFile");
      goto done;
    }
    else if (read_cnt == 0)
    {
      success = true;
      goto done;
    }
    read_cnt /= sizeof(short);
    
    written = writeAudio(buf, read_cnt);
    if (written == -1)
    {
      perror("write in MsgHandler::writeFromFile");
      goto done;
    }
    //printf("Read=%d  Written=%d\n", read_cnt, written);
  } while (written == read_cnt);
  
  //printf("lseeking...\n");
  lseek(file, written*sizeof(short) - read_cnt*sizeof(short), SEEK_CUR);
  
  return;
    
  done:
    //printf("Done...\n");
    ::close(file);
    file = -1;
    
    msg_queue.pop_front();
    if (msg_queue.empty())
    {
      allMsgsWritten();
    }
    else
    {
      playNextMsg();
    }
} /* MsgHandler::writeFromFile */



