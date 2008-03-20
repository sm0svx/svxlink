#include <string>
#include <list>
#include <map>

#include <sigc++/sigc++.h>

#include <AsyncAudioSource.h>

class MsgHandler : public SigC::Object, public Async::AudioSource
{
  public:
    MsgHandler(const std::string& base_dir, int sample_rate);
    ~MsgHandler(void);
    
    void playFile(const std::string& path);
    void playMsg(const std::string& context, const std::string& msg);
    void playNumber(float number);
    void spellWord(const std::string& word);
    void playSilence(int length);
    bool isWritingMessage(void) const { return !msg_queue.empty(); }
    void clear(void);
    void begin(void);
    void end(void);
    
    SigC::Signal0<void>       	    allMsgsWritten;
    
    void resumeOutput(void);
    
    void allSamplesFlushed(void) {}
    
    
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
    bool      	      	    play_next;
    bool      	      	    pending_play_next;
    
    void playNextMsg(void);
    void executeCmd(const std::string& cmd);
    void writeFromFile(void);
    int readSamples(float *samples, int len);
    void unreadSamples(int len);


};
