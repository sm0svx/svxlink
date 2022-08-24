#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncTcpClient.h>

using namespace Async;

class MyClass : public sigc::trackable
{
  public:
    MyClass(void)
    {
      con.connected.connect(mem_fun(*this, &MyClass::onConnected));
      con.disconnected.connect(mem_fun(*this, &MyClass::onDisconnected));
      con.dataReceived.connect(mem_fun(*this, &MyClass::onDataReceived));
      con.connect("www.svxlink.org", 80);
    }

  private:
    TcpClient<> con;

    void onConnected(void)
    {
      std::cout << "--- Connection established to " << con.remoteHost()
                << std::endl;
      std::string req(
          "GET / HTTP/1.0\r\n"
          "Host: " + con.remoteHostName() + "\r\n"
          "\r\n");
      std::cout << "--- Sending request:\n" << req << std::endl;
      con.write(req.data(), req.size());
    }

    void onDisconnected(TcpConnection *, TcpClient<>::DisconnectReason reason)
    {
      std::cout << "--- Disconnected from " << con.remoteHost() << ": "
                << TcpConnection::disconnectReasonStr(reason)
                << std::endl;
      Application::app().quit();
    }

    int onDataReceived(TcpConnection *, void *buf, int count)
    {
      std::cout << "--- Data received:" << std::endl;
      const char *str = static_cast<char *>(buf);
      std::string html(str, str+count);
      std::cout << html;
      return count;
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}
