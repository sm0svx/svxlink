#include <dlfcn.h>
#include <sigc++/signal_system.h>

#include <string>
#include <list>

namespace Async
{
  class Config;
};

class Logic;

class Module : public SigC::Object
{
  public:
    typedef Module* (*InitFunc)(void *dl_handle, Logic *logic, int id,
      	      	      	        const char *cfg_name);
    
    Module(void *dl_handle, Logic *logic, int id);
    virtual ~Module(void) { }
    
    void *pluginHandle(void) const { return m_dl_handle; }
    int id(void) const { return m_id; }
    Logic *logic(void) const { return m_logic; }
    void activate(void);
    void deactivate(void);
    bool isActive(void) const { return m_is_active; }
    bool isTransmitting(void) const { return m_is_transmitting; }
    Async::Config &cfg(void) const;
    
    virtual const char *name(void) const = 0;
    virtual void activateInit(void) = 0;
    virtual void deactivateCleanup(void) = 0;
    virtual void dtmfDigitReceived(char digit) {}
    virtual void dtmfCmdReceived(const std::string& cmd) {}
    virtual void playModuleName(void) { playMsg("name"); }
    virtual void playHelpMsg(void) { playMsg("help"); }
    virtual void squelchOpen(bool is_open) {}
    virtual int audioFromRx(short *samples, int count) { return count; }
    
    
  protected:
    void playMsg(const std::string& msg) const;
    void playNumber(int number) const;
    void spellWord(const std::string& word) const;
    int audioFromModule(short *samples, int count);
    void transmit(bool tx);
    bool activateMe(void);
    void deactivateMe(void);
    Module *findModule(int id);
    std::list<Module*> moduleList(void);
    const std::string& logicName(void) const;

  private:
    void      	      *m_dl_handle;
    Logic     	      *m_logic;
    int       	      m_id;
    bool      	      m_is_transmitting;
    SigC::Connection  m_audio_con;
    SigC::Connection  m_squelch_con;
    bool      	      m_is_active;
    
};

