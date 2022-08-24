#include <iostream>
#include "DemoPluginBase.h"

class DemoPlugin : public DemoPluginBase
{
  public:
    DemoPlugin(void)
    {
      std::cout << "### DemoPlugin::DemoPlugin" << std::endl;
    }
    virtual bool initialize(const std::string& str) override
    {
      std::cout << "### DemoPlugin::initialize: str=" << str << std::endl;
      return true;
    }
  protected:
    virtual ~DemoPlugin(void) override
    {
      std::cout << "### DemoPlugin::~DemoPlugin" << std::endl;
    }
};

extern "C" {
  DemoPluginBase* construct(void) { return new DemoPlugin; }
}
