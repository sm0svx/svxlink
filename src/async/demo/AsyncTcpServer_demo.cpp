#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncTcpServer.h>

using namespace std;
using namespace Async;

class MyClass : public sigc::trackable
{
  public:
    MyClass(void)
    {
      server = new TcpServer<>("12345");
      server->clientConnected.connect(
        mem_fun(*this, &MyClass::onClientConnected));
      server->clientDisconnected.connect(
      	      mem_fun(*this, &MyClass::onClientDisconnected));
      cout << "Connect using: \"telnet localhost 12345\" from "
      	      "another console\n";
    }
    
    ~MyClass(void)
    {
      delete server;
    }

  private:
    TcpServer<>* server;
    
    void onClientConnected(TcpConnection *con)
    {
      cout << "Client " << con->remoteHost() << ":"
      	   << con->remotePort() << " connected, "
           << server->numberOfClients() << " clients connected\n";
      	// We need ONLY to add signal for receive data to the TcpConnection
      con->dataReceived.connect(mem_fun(*this, &MyClass::onDataReceived));
      	// Send welcome message to the connected client */
      con->write("Hello, client!\n", 15);
    }
    
    void onClientDisconnected(TcpConnection *con,
      	      	              TcpConnection::DisconnectReason reason)
    {
      cout << "Client " << con->remoteHost().toString() << ":"
      	   << con->remotePort() << " disconnected,"
           << server->numberOfClients() << " clients connected\n";
      /* Don't delete the con object, the TcpServer will do it */
    }
    
    int onDataReceived(TcpConnection *con, void *buf, int count)
    {
      	// retreive data
      char *str = static_cast<char *>(buf);
      string data(str, str+count);
      cout << data;
      
      	// Send data back to sender
      string dataOut = string("You said: ") + data;
      server->writeOnly(con, dataOut.c_str(), dataOut.size());
      
      	// Other way to send to sender
      //con->write(dataOut.c_str(), dataOut.size());
      
      	// Send to other clients if there is more then one connected to server
      if (server->numberOfClients() > 1)
      {
          // Send data back to all OTHER clients
        dataOut = string("He said : ") + data;
        server->writeExcept(con, dataOut.c_str(), dataOut.size());
	
          // Send data back to all clients
        dataOut = string("To all  : ") + data;
        server->writeAll(dataOut.c_str(), dataOut.size());
      }
      return count;
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}


