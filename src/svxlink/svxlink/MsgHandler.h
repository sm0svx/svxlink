#include <string>
#include <list>
#include <map>

#include <sigc++/signal_system.h>


class MsgHandler : public SigC::Object
{
  public:
    MsgHandler(const std::string& base_dir);
    ~MsgHandler(void);
    
    void playMsg(const std::string& context, const std::string& msg);
    void playNumber(int number);
    void spellWord(const std::string& word);
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
    
    void playNextMsg(void);
    void writeFromFile(void);


};
