#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncTcpClient.h>

using namespace std;
using namespace Async;

class MyClass : public sigc::trackable
{
  public:
    MyClass(void)
    {
      con = new TcpClient<>("www.linux.org", 80);
      con->connected.connect(mem_fun(*this, &MyClass::onConnected));
      con->disconnected.connect(mem_fun(*this, &MyClass::onDisconnected));
      con->dataReceived.connect(mem_fun(*this, &MyClass::onDataReceived));
      con->connect();
    }
    
    ~MyClass(void)
    {
      delete con;
    }

  private:
    TcpClient<>* con;
    
    void onConnected(void)
    {
      cout << "Connection established to " << con->remoteHost() << "...\n";
      con->write("GET /\n", 6);
    }
    
    void onDisconnected(TcpConnection *con, TcpClient<>::DisconnectReason reason)
    {
      cout << "Disconnected from " << con->remoteHost() << "...\n";
      Application::app().quit();
    }
    
    int onDataReceived(TcpConnection *con, void *buf, int count)
    {
      char *str = static_cast<char *>(buf);
      string html(str, str+count);
      cout << html;
      return count;
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}
