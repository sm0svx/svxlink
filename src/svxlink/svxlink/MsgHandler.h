#include <string>
#include <list>
#include <map>

#include <sigc++/signal_system.h>


class MsgHandler : public SigC::Object
{
  public:
    MsgHandler(const std::string& base_dir, int sample_rate);
    ~MsgHandler(void);
    
    void playFile(const std::string& path);
    void playMsg(const std::string& context, const std::string& msg);
    void playNumber(int number);
    void spellWord(const std::string& word);
    void playSilence(int length);
    void writeBufferFull(bool is_full);
    bool isWritingMessage(void) const { return !msg_queue.empty(); }
    void clear(void);
    
    SigC::Signal2<int, short*, int> writeAudio;
    SigC::Signal0<void>       	    allMsgsWritten;
    
    
  private:
    class MsgQueueItem
    {
      public:
      	std::string context;
      	std::string msg;
      	MsgQueueItem(const std::string& context, const std::string& msg)
	  : context(context), msg(msg)
	{
	}
    };
    
    std::list<MsgQueueItem> msg_queue;
    int       	      	    file;
    std::string       	    base_dir;
    int			    silence_left;
    int			    sample_rate;
    
    void playNextMsg(void);
    void executeCmd(const std::string& cmd);
    void writeFromFile(void);
    int readSamples(short *samples, int len);
    void unreadSamples(int len);


};
