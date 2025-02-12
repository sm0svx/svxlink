#include <iostream>
#include <sstream>
#include <AsyncCppApplication.h>
#include <AsyncTcpClient.h>

using namespace Async;

class MyClass : public sigc::trackable
{
  public:
    MyClass(std::string hostname, uint16_t port)
    {
      con.connected.connect(mem_fun(*this, &MyClass::onConnected));
      con.disconnected.connect(mem_fun(*this, &MyClass::onDisconnected));
      con.dataReceived.connect(mem_fun(*this, &MyClass::onDataReceived));
      con.connect(hostname, port);
    }

  private:
    TcpClient<> con;

    void onConnected(void)
    {
      std::cout << "--- Connection established to " << con.remoteHost()
                << std::endl;
      std::string req(
          "GET / HTTP/1.0\r\n"
          "Connection: Close\r\n"
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
      std::cout.write(static_cast<char*>(buf), count);
      std::cout << std::endl;
      return count;
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class("checkip.amazonaws.com", 80);
  app.exec();
}
