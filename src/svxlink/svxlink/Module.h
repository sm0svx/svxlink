#include <dlfcn.h>
#include <sigc++/signal_system.h>

#include <string>
#include <list>

namespace Async
{
  class Config;
  class Timer;
};

class Logic;

class Module : public SigC::Object
{
  public:
    typedef Module* (*InitFunc)(void *dl_handle, Logic *logic,
      	      	      	        const char *cfg_name);
    
    Module(void *dl_handle, Logic *logic, const std::string& cfg_name);
    virtual ~Module(void);
    
    virtual bool initialize(void);

    const std::string cfgName(void) const { return m_cfg_name; }
    void *pluginHandle(void) const { return m_dl_handle; }
    int id(void) const { return m_id; }
    Logic *logic(void) const { return m_logic; }
    void activate(void);
    void deactivate(void);
    bool isActive(void) const { return m_is_active; }
    bool isTransmitting(void) const { return m_is_transmitting; }
    Async::Config &cfg(void) const;
    const std::string& logicName(void) const;
    void playModuleName(void);
    void playHelpMsg(void);
    
    virtual const char *name(void) const = 0;
    virtual void activateInit(void) = 0;
    virtual void deactivateCleanup(void) = 0;
    virtual void dtmfDigitReceived(char digit) {}
    virtual void dtmfCmdReceived(const std::string& cmd) {}
    virtual void squelchOpen(bool is_open) {}
    virtual int audioFromRx(short *samples, int count) { return count; }
    virtual void allMsgsWritten(void) {}
    
    void playMsg(const std::string& msg) const;
    void playNumber(int number) const;
    void spellWord(const std::string& word) const;
    void playSilence(int length) const;
    
  protected:
    int audioFromModule(short *samples, int count);
    void transmit(bool tx);
    bool activateMe(void);
    void deactivateMe(void);
    Module *findModule(int id);
    std::list<Module*> moduleList(void);
    void setIdle(bool is_idle);

  private:
    void      	      *m_dl_handle;
    Logic     	      *m_logic;
    int       	      m_id;
    bool      	      m_is_transmitting;
    SigC::Connection  m_audio_con;
    SigC::Connection  m_squelch_con;
    bool      	      m_is_active;
    std::string	      m_cfg_name;
    Async::Timer      *m_tmo_timer;
    
    void moduleTimeout(Async::Timer *t);

};

