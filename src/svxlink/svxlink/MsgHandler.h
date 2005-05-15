#include <string>
#include <list>
#include <map>

#include <sigc++/signal_system.h>


class QueueItem;


class MsgHandler : public SigC::Object
{
  public:
    MsgHandler(int sample_rate);
    ~MsgHandler(void);
    
    void playFile(const std::string& path);
    void playSilence(int length);
    void playTone(int fq, int amp, int length);

    void writeBufferFull(bool is_full);
    bool isWritingMessage(void) const { return !msg_queue.empty(); }
    void clear(void);
    void begin(void);
    void end(void);
    
    SigC::Signal2<int, short*, int> writeAudio;
    SigC::Signal0<void>       	    allMsgsWritten;
    
    
  private:
    std::list<QueueItem*>   msg_queue;
    int			    sample_rate;
    int      	      	    nesting_level;
    bool      	      	    pending_play_next;
    
    void addItemToQueue(QueueItem *item);
    void playMsg(void);
    void writeSamples(void);

};
