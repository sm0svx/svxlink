#include <iostream>
#include <cassert>
#include <AsyncCppApplication.h>
#include <AsyncTcpServer.h>
#include <AsyncFramedTcpConnection.h>

using namespace std;
using namespace Async;

class MyClass : public sigc::trackable
{
  public:
    MyClass(void)
    {
      server = new TcpServer<FramedTcpConnection>("12345");
      server->clientConnected.connect(
          mem_fun(*this, &MyClass::onClientConnected));
      server->clientDisconnected.connect(
          mem_fun(*this, &MyClass::onClientDisconnected));
      cout << "Start AsyncFramedTcpClient_demo in another window to transfer "
              "some frames to the server" << endl;
    }

    ~MyClass(void)
    {
      delete server;
    }

  private:
    TcpServer<FramedTcpConnection> *server;

    void onClientConnected(FramedTcpConnection *con)
    {
      cout << "Client " << con->remoteHost() << ":"
           << con->remotePort() << " connected, "
           << server->numberOfClients() << " clients connected\n";
      con->frameReceived.connect(mem_fun(*this, &MyClass::onFrameReceived));
    }

    void onClientDisconnected(FramedTcpConnection *con,
                              FramedTcpConnection::DisconnectReason reason)
    {
      cout << "Client " << con->remoteHost().toString() << ":"
           << con->remotePort() << " disconnected,"
           << server->numberOfClients() << " clients connected\n";
      /* Don't delete the con object, the TcpServer will do it */
    }

    void onFrameReceived(FramedTcpConnection *con, std::vector<uint8_t>& buf)
    {
      cout << "Received a frame with " << buf.size() << " bytes of data\n";
      if (buf.size() == 4)
      {
        std::string cmd(buf.begin(), buf.end());
        cout << "Quitting!\n";
        Application::app().quit();
      }
      else
      {
          // Check content. Should be a repeating 0-255 series.
        uint8_t next = 0;
        for (size_t i=0; i<buf.size(); ++i)
        {
          assert(buf[i] == next++);
        }
      }
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}


