#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncTcpClient.h>
#include <AsyncFramedTcpConnection.h>

using namespace std;
using namespace Async;

class MyClass : public sigc::trackable
{
  public:
    MyClass(void)
    {
      con = new TcpClient<FramedTcpConnection>("localhost", 12345);
      con->connected.connect(mem_fun(*this, &MyClass::onConnected));
      con->disconnected.connect(mem_fun(*this, &MyClass::onDisconnected));
      con->frameReceived.connect(mem_fun(*this, &MyClass::onFrameReceived));
      con->connect();
    }

    ~MyClass(void)
    {
      delete con;
    }

  private:
    TcpClient<FramedTcpConnection> *con;

    void onConnected(void)
    {
      cout << "Connection established to " << con->remoteHost() << "...\n";
      size_t bufsize = 10000000;
      char *buf = new char[bufsize];
      for (size_t i=0; i<bufsize; ++i) { buf[i] = i & 0xff; }
      cout << "Sending 3x frames to the server..." << endl;
      con->write(buf, bufsize/4);
      con->write(buf, bufsize/2);
      con->write(buf, bufsize);
      cout << "Sending QUIT to server" << endl;
      con->write("QUIT", 4);
      delete [] buf;
    }

    void onDisconnected(FramedTcpConnection *con,
                        TcpClient<FramedTcpConnection>::DisconnectReason reason)
    {
      cout << "Disconnected from " << con->remoteHost() << "...\n";
      Application::app().quit();
    }

    void onFrameReceived(FramedTcpConnection *con, vector<uint8_t>& frame)
    {
      char *str = reinterpret_cast<char *>(frame.data());
      string html(str, str+frame.size());
      cout << html;
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}
