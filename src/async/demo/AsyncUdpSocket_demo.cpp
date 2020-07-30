#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncUdpSocket.h>
#include <AsyncIpAddress.h>

using namespace std;
using namespace Async;

class MyClass : public sigc::trackable
{
  public:
    MyClass(void)
    {
      sock = new UdpSocket(12345);
      sock->dataReceived.connect(mem_fun(*this, &MyClass::onDataReceived));
      IpAddress addr("127.0.0.1");
      sock->write(addr, 12345, "Hello, UDP!\n", 13);
    }
    
    ~MyClass(void)
    {
      delete sock;
    }

  private:
    UdpSocket * sock;
    
    void onDataReceived(const IpAddress& addr, uint16_t port,
                        void *buf, int count)
    {
      cout << "Data received from " << addr << ":" << port << ": "
           << static_cast<char *>(buf);
      Application::app().quit();
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}
