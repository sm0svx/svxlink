#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncTcpPrioClient.h>

using namespace std;
using namespace Async;

class MyClass : public sigc::trackable
{
  public:
    MyClass(void)
    {
      con = new TcpPrioClient<>;
      con->connected.connect(mem_fun(*this, &MyClass::onConnected));
      con->disconnected.connect(mem_fun(*this, &MyClass::onDisconnected));
      con->dataReceived.connect(mem_fun(*this, &MyClass::onDataReceived));
      con->setService("svxreflector", "tcp", "test.svxlink.org");
      con->addStaticSRVRecord(3600, 1, 100, 5304, "localhost.");
      con->setBindIp(Async::IpAddress("127.0.0.1"));
      con->connect();
    }

    ~MyClass(void)
    {
      delete con;
    }

  private:
    TcpPrioClient<>* con;

    void onConnected(void)
    {
      std::cout << "Connection established to " << con->remoteHost()
                << ":" << con->remotePort() << "..." << std::endl;
      //con->write("GET /\n", 6);
    }

    void onDisconnected(TcpConnection *con, TcpClient<>::DisconnectReason reason)
    {
      std::cout << "Disconnected from " << con->remoteHost()
                << ":" << con->remotePort()
                << ": " << con->disconnectReasonStr(reason)
                << "..." << std::endl;
    }

    int onDataReceived(TcpConnection *con, void *buf, int count)
    {
      char *str = static_cast<char *>(buf);
      string html(str, str+count);
      cout << html;
      std::cout << "### Quitting: count=" << count << std::endl;
      Application::app().quit();
      return count;
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}
