#include <iostream>
#include <AsyncPlugin.h>

// Base class for all plugins of type DemoPlugin
class DemoPluginBase : public Async::Plugin
{
  public:
    // Return the name of this plugin type
    static std::string typeName(void) { return "DemoPlugin"; }

    // Plugin base class constructor
    DemoPluginBase(void)
    {
      std::cout << "### DemoPluginBase::DemoPluginBase" << std::endl;
    }

    // Any plugin type specific functions
    virtual bool initialize(const std::string& str) = 0;

  protected:
    // Plugin base class destructor
    virtual ~DemoPluginBase(void) override
    {
      std::cout << "### DemoPluginBase::~DemoPluginBase" << std::endl;
    }
};
