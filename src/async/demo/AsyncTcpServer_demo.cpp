#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncTcpServer.h>

using namespace std;
using namespace Async;

class MyClass : public SigC::Object
{
  public:
    MyClass(void)
    {
      server = new TcpServer("12345");
      server->clientConnected.connect(slot(this, &MyClass::onClientConnected));
    }
    
    ~MyClass(void)
    {
      delete server;
    }

  private:
    TcpServer *server;
    
    void onClientConnected(TcpConnection *con)
    {
      cout << "Client " << con->remoteHost() << ":"
      	   << con->remotePort() << " connected...\n";
      con->disconnected.connect(slot(this, &MyClass::onDisconnected));
      con->dataReceived.connect(slot(this, &MyClass::onDataReceived));
      con->write("Hello, client!\n", 15);
    }
    
    void onDisconnected(TcpConnection *con,
      	      	      	TcpConnection::DisconnectReason reason)
    {
      cout << "Client " << con->remoteHost().toString() << ":"
      	   << con->remotePort() << " disconnected...\n";
      delete con;
    }
    
    int onDataReceived(TcpConnection *con, void *buf, int count)
    {
      char *str = static_cast<char *>(buf);
      string data(str, str+count);
      cout << data;
      data = string("You said: ") + data;
      con->write(data.c_str(), data.size());
      return count;
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}
