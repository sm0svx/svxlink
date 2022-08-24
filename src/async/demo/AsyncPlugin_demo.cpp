#include <iostream>
#include "DemoPluginBase.h"

int main(void)
{
  auto p = Async::Plugin::load<DemoPluginBase>("DemoPlugin.so");
  if (p == nullptr)
  {
    exit(1);
  }
  std::cout << "Found plugin " << p->pluginPath() << std::endl;
  p->initialize("hello");
  Async::Plugin::unload(p);
  p = nullptr;
  return 0;
}
