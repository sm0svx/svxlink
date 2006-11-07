#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncUdpSocket.h>
#include <AsyncIpAddress.h>

using namespace std;
using namespace Async;

class MyClass : public SigC::Object
{
  public:
    MyClass(void)
    {
      sock = new UdpSocket(12345);
      sock->dataReceived.connect(slot(*this, &MyClass::onDataReceived));
      IpAddress addr("127.0.0.1");
      sock->write(addr, 12345, "Hello, UDP!\n", 13);
    }
    
    ~MyClass(void)
    {
      delete sock;
    }

  private:
    UdpSocket * sock;
    
    void onDataReceived(const IpAddress& addr, void *buf, int count)
    {
      cout << "Data received from " << addr << ": " << static_cast<char *>(buf);
      Application::app().quit();
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}
