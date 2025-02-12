/******************************************************************************
# To generate the server key- and certificate files:
openssl req -x509 -newkey rsa:2048 -nodes -keyout server.key \
  -subj "/CN=localhost" -sha256 -days 3650 -out server.crt
******************************************************************************/

#include <openssl/x509_vfy.h>
#include <openssl/x509.h>

#include <iostream>

#include <AsyncCppApplication.h>
#include <AsyncTcpServer.h>
#include <AsyncTimer.h>

class MySslServer
{
  public:
    MySslServer(const std::string& service)
      : m_server(service), m_ping_timer(1000, Async::Timer::TYPE_PERIODIC)
    {
      if (!m_ssl_ctx.setCertificateFiles("server.key", "server.crt"))
      {
        std::cout << "*** ERROR: Failed to read key/cert files" << std::endl;
        exit(1);
      }
      if (!m_ssl_ctx.setCaCertificateFile("server.crt"))
      {
        std::cout << "*** ERROR: Failed to read CA file" << std::endl;
        exit(1);
      }

      std::string session_id("AsyncSslTcpServer_demo");
      SSL_CTX_set_session_id_context(
          m_ssl_ctx,
          reinterpret_cast<const unsigned char*>(session_id.data()),
          session_id.size());

      m_server.setSslContext(m_ssl_ctx);
      m_server.clientConnected.connect(
          sigc::mem_fun(*this, &MySslServer::onClientConnected));

        // Start a timer to periofically send some data to all clients
      //m_ping_timer.expired.connect(
      //    [&](Async::Timer*)
      //    {
      //      m_server.writeAll("PING\n", 5);
      //    });
      std::cout << "Connect using: \"openssl s_client -connect localhost:"
                << service << "\" " "from another console" << std::endl;
    }

  private:
    Async::TcpServer<Async::TcpConnection>  m_server;
    Async::SslContext                       m_ssl_ctx;
    Async::Timer                            m_ping_timer;

    void onClientConnected(Async::TcpConnection *con)
    {
      std::cout << "Client " << con->remoteHost() << ":"
             << con->remotePort() << " connected, "
             << m_server.numberOfClients() << " clients connected\n";
      con->enableSsl(true);
      con->verifyPeer.connect(
        [](Async::TcpConnection* con, bool preverify_ok,
           X509_STORE_CTX *ctx)
        {
          X509* err_cert = X509_STORE_CTX_get_current_cert(ctx);
          assert(err_cert != nullptr);
          int err = X509_STORE_CTX_get_error(ctx);
          int depth = X509_STORE_CTX_get_error_depth(ctx);

          /*
           * Retrieve the pointer to the SSL of the connection currently treated
           * and the application specific data stored into the SSL object.
           */
          //SSL* ssl = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());

          char buf[256];
          auto subj = X509_get_subject_name(err_cert);
          int lastpos = -1;
          for (;;)
          {
              lastpos = X509_NAME_get_index_by_NID(subj,
                  NID_commonName, lastpos);
              if (lastpos == -1)
              {
                  break;
              }
              X509_NAME_ENTRY *e = X509_NAME_get_entry(subj, lastpos);
              ASN1_STRING *d = X509_NAME_ENTRY_get_data(e);
              const unsigned char* str = (ASN1_STRING_get0_data(d));
              std::cout << "### CN=" << str << std::endl;
          }
          X509_NAME_oneline(subj, buf, sizeof(buf));


          /*
           * Catch a too long certificate chain. The depth limit set using
           * SSL_CTX_set_verify_depth() is by purpose set to "limit+1" so
           * that whenever the "depth>verify_depth" condition is met, we
           * have violated the limit and want to log this error condition.
           * We must do it here, because the CHAIN_TOO_LONG error would not
           * be found explicitly; only errors introduced by cutting off the
           * additional certificates would be logged.
           */
          if (depth > 1)
          {
            preverify_ok = false;
            err = X509_V_ERR_CERT_CHAIN_TOO_LONG;
            X509_STORE_CTX_set_error(ctx, err);
          }
          if (!preverify_ok)
          {
            std::cout << "*** ERROR: Verify error:num=" << err << ":"
                      << X509_verify_cert_error_string(err) << ":depth="
                      << depth << ":" << buf << std::endl;
          }
          else if (true)
          {
            std::cout << "### depth=" << depth << ":" << buf << std::endl;
          }

          /*
           * At this point, err contains the last verification error. We can use
           * it for something special
           */
          if (!preverify_ok && (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT))
          {
            X509_NAME_oneline(X509_get_issuer_name(err_cert),
                buf, sizeof(buf));
            std::cout << "### issuer=" << buf << std::endl;
          }

          return preverify_ok;
        });
      con->dataReceived.connect(
          [](Async::TcpConnection* con, void* buf, int len)
          {
            std::cout << "SSL data received: len=" << len;
            std::cout << " data=";
            std::cout.write(reinterpret_cast<char*>(buf), len);
            if (memcmp(buf, "GET ", 4) == 0)
            {
              std::string res;
              res += "HTTP/1.1 200 OK\r\n";
              res += "Content-Type: text/plain\r\n";
              res += "Content-Length: 15\r\n";
              res += "Connection: Close\r\n";
              res += "\r\n";
              res += "Hello, Client\r\n";
              con->write(res.data(), res.size());
            }
            return len;
          });
      con->disconnected.connect(
          [&](Async::TcpConnection *con,
              Async::TcpConnection::DisconnectReason reason)
          {
            std::cout << "Client " << con->remoteHost().toString() << ":"
                      << con->remotePort() << " disconnected, "
                      << m_server.numberOfClients() << " clients connected\n";
          });
    }
}; /* class MySslServer */

int main(int argc, char **argv)
{
  Async::CppApplication app;
  app.unixSignalCaught.connect([&](int signal)
      {
        std::cout << "Caught signal. Exiting..." << std::endl;
        app.quit();
      });
  app.catchUnixSignal(SIGINT);
  app.catchUnixSignal(SIGTERM);

  MySslServer ssl_server("12345");

  app.exec();

  return 0;
} /* main */
