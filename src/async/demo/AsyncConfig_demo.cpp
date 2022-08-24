#include <iostream>
#include <string>
#include <cstdlib>

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

    // Read the string value without checking if it exists or not.
  cout << "value=" << cfg.getValue("SECTION1", "VALUE1") << endl;
  
    // Read the string value, returning it in a variable.
    // The return value will indicate if the variable was found or not.
  string str_val;
  if (cfg.getValue("SECTION1", "VALUE2", str_val))
  {
    cout << "str_val=" << str_val << endl;
  }
  else
  {
    cerr << "*** ERROR: Config variable SECTION1/VALUE2 not found.\n";
  }
  
    // Read an integer value.
  int int_val = 0;
  if (cfg.getValue("SECTION2", "MY_INT", int_val))
  {
    cout << "int_val=" << int_val << endl;
  }
  else
  {
    cerr << "*** ERROR: Config variable SECTION2/MY_INT malformed or "
	    "not found.\n";
  }
  
    // Read a char value. Missing value is OK.
  char char_val = 'Q';
  if (cfg.getValue("SECTION1", "NO_VALUE", char_val, true))
  {
    cout << "char_val=" << char_val << endl;
  }
  else
  {
    cerr << "*** ERROR: Config variable SECTION1/NO_VALUE malformed.\n";
  }
  
    // Read a float with min and max limits.
  float float_val = 0.0;
  if (cfg.getValue("SECTION2", "MY_FLOAT", 3.0f, 4.0f, float_val))
  {
    cout << "float_val=" << float_val << endl;
  }
  else
  {
    cerr << "*** ERROR: Config variable SECTION2/MY_FLOAT malformed, "
	    "not found or out of range.\n";
  }
}
