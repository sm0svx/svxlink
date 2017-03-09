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
      sock = new UdpSocket(62030);
      sock->dataReceived.connect(mem_fun(*this, &MyClass::onDataReceived));
      IpAddress addr("87.106.126.49");
      std::cout << "sending: RPTL0027FA97\n";
      sock->write(addr, 62030, "RPTL0027FA97", 12);
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
