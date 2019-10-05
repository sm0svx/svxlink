#include <sstream>
#include <AsyncCppApplication.h>
#include <AsyncHttpServerConnection.h>
#include <AsyncTcpServer.h>

void requestReceived(Async::HttpServerConnection *con,
                     Async::HttpServerConnection::Request& req)
{
  std::cout << "--- " << req.method << " " << req.target << std::endl;

  Async::HttpServerConnection::Response res;
  if ((req.method != "GET") && (req.method != "HEAD"))
  {
    res.setCode(501);
    res.setContent("text/plain", req.method + ": Method not implemented");
    con->write(res);
    return;
  }

  std::ostringstream os;
  os << "{"
     << "\"method\":\"" << req.method << "\","
     << "\"target\":\"" << req.target << "\","
     << "\"client-proto-major\":" << req.ver_major << ","
     << "\"client-proto-minor\":" << req.ver_minor << ","
     << "\"headers\":{";
  Async::HttpServerConnection::Headers::const_iterator it;
  for (it=req.headers.begin(); it!=req.headers.end(); ++it)
  {
    std::cout << (*it).first << ": " << (*it).second << std::endl;
    if (it != req.headers.begin())
    {
      os << ",";
    }
    os << "\"" << (*it).first << "\":\"" << (*it).second << "\"";
  }
  os << "}}";
  res.setContent("application/json", os.str());
  if (req.method == "HEAD")
  {
    res.setSendContent(false);
  }
  res.setCode(200);
  con->write(res);
} /* requestReceived */


void clientConnected(Async::HttpServerConnection *con)
{
  std::cout << "/// Client connected: "
            << con->remoteHost() << ":" << con->remotePort() << std::endl;
  con->requestReceived.connect(sigc::ptr_fun(&requestReceived));
} /* clientConnected */


void clientDisconnected(Async::HttpServerConnection *con,
                        Async::HttpServerConnection::DisconnectReason reason)
{
  std::cout << "\\\\\\ Client disconnected: "
            << con->remoteHost() << ":" << con->remotePort()
            << ": " << Async::HttpServerConnection::disconnectReasonStr(reason)
            << std::endl;
} /* clientConnected */


int main(void)
{
  Async::CppApplication app;
  app.catchUnixSignal(SIGINT);
  app.catchUnixSignal(SIGTERM);
  app.unixSignalCaught.connect(
      sigc::hide(sigc::mem_fun(app, &Async::CppApplication::quit)));
  Async::TcpServer<Async::HttpServerConnection> server("8080");
  server.clientConnected.connect(sigc::ptr_fun(&clientConnected));
  server.clientDisconnected.connect(sigc::ptr_fun(&clientDisconnected));
  app.exec();
}
