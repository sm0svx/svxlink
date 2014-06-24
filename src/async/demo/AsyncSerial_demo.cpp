#include <iostream>
#include <cstdlib>
#include <cstdio>

#include <AsyncCppApplication.h>
#include <AsyncSerial.h>

using namespace std;
using namespace Async;

class MyClass : public sigc::trackable
{
  public:
    MyClass(void)
    {
      serial = new Serial("/dev/ttyS0");
      serial->charactersReceived.connect(
      	  mem_fun(*this, &MyClass::onCharactersReceived));

      if (!serial->open(true))
      {
      	perror("Open serial port failed");
	exit(1);
      }
      serial->setParams(9600, Serial::PARITY_NONE, 8, 1, Serial::FLOW_NONE);
      
      serial->write("Hello, serial\n", 14);
    }
    
    ~MyClass(void)
    {
      serial->close();
      delete serial;
    }

  private:
    Serial *serial;
    
    void onCharactersReceived(char *buf, int count)
    {
      cout << "Read " << count << " characters from serial port: "
      	   << buf << endl;
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}
