#include <iostream>
#include <string>

#include <AsyncConfig.h>

using namespace std;
using namespace Async;

int main(int argc, char **argv)
{
  Config cfg;
  if (!cfg.open("test.cfg"))
  {
    cerr << "*** Error: Could not open config file test.cfg\n";
    exit(1);
  }

  string value;
  if (cfg.getValue("SECTION1", "VALUE2", value))
  {
    cout << ">>> value=" << value << endl;
  }
  else
  {
    cerr << "*** Error: Could not find config variable SECTION1/VALUE2\n";
    exit(1);
  }
}
