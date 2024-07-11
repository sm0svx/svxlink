/******************************************************************************
# To generate the client key- and certificate files:
openssl req -newkey rsa:2048 -nodes -keyout client.key \
  -subj "/CN=MYCALL" -sha256 -days 3650 -out client.csr

# Sign the client certificat using the server key
openssl x509 -req -in client.csr -CA server.crt \
  -CAkey server.key -CAcreateserial -days 3650 -out client.crt
******************************************************************************/

#include <iostream>
#include <sstream>
#include <AsyncCppApplication.h>
#include <AsyncTcpClient.h>

using namespace std;
using namespace Async;

class MyClass : public sigc::trackable
{
  public:
    MyClass(std::string hostname, uint16_t port) : hostname(hostname)
    {
      con = new TcpClient<>(hostname, port);
      if (!m_ssl_ctx.setCertificateFiles("client.key", "client.crt"))
      {
        std::cout << "*** ERROR: Failed to read key/cert files" << std::endl;
        exit(1);
      }
      if (!m_ssl_ctx.setCaCertificateFile("server.crt"))
      {
        std::cout << "*** ERROR: Failed to read CA file" << std::endl;
        exit(1);
      }
      con->setSslContext(m_ssl_ctx);
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
    std::string  hostname;
    SslContext   m_ssl_ctx;

    void onConnected(void)
    {
      cout << "Connection established to " << con->remoteHost() << "...\n";
      con->enableSsl(true);
      std::ostringstream os;
      os << "GET / HTTP/1.0\r\n"
            "Connection: Close\r\n"
            "Host: " << hostname << "\r\n"
            "\r\n";
      con->write(os.str().data(), os.str().size());
    }

    void onDisconnected(TcpConnection *con, TcpClient<>::DisconnectReason reason)
    {
      cout << "Disconnected from " << con->remoteHost() << "...\n";
      Application::app().quit();
    }

    int onDataReceived(TcpConnection *con, void *buf, int count)
    {
      cout.write(static_cast<char*>(buf), count);
      cout << std::endl;
      Application::app().quit();
      return count;
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  if (argc < 3)
  {
    std::cout << "Usage: " << argv[0] << " <hostname> <port>" << std::endl;
    exit(1);
  }
  MyClass my_class(argv[1], atoi(argv[2]));
  app.exec();
}
