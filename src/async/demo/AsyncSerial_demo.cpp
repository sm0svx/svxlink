#include <iostream>

#include <AsyncCppApplication.h>
#include <AsyncSerial.h>

using namespace std;
using namespace Async;

class MyClass : public SigC::Object
{
  public:
    MyClass(void)
    {
      serial = new Serial("/dev/ttyS0", 9600, Serial::PARITY_NONE, 8, 1,
      	      	      	  Serial::FLOW_NONE);
      serial->charactersReceived.connect(
      	  slot(this, &MyClass::onCharactersReceived));

      if (!serial->open())
      {
      	perror("Open serial port failed");
	exit(1);
      }
      
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
