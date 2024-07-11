/**
@file	 AsyncTcpConnection.cpp
@brief   Contains a class for handling exiting TCP connections
@author  Tobias Blomberg
@date	 2003-04-12

This file contains a class to handle exiting TCP connections
to a remote host. See usage instructions in the class definition.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2022 Tobias Blomberg

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <algorithm>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncFdWatch.h"
#include "AsyncDnsLookup.h"
#include "AsyncTcpConnection.h"
#include "AsyncSslX509.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/

std::map<SSL*, TcpConnection*> TcpConnection::ssl_con_map;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

const char *TcpConnection::disconnectReasonStr(DisconnectReason reason)
{
  switch (reason)
  {
    case DR_HOST_NOT_FOUND:
      return "Host not found";

    case DR_REMOTE_DISCONNECTED:
      return "Connection closed by remote peer";

    case DR_SYSTEM_ERROR:
      return strerror(errno);

    case DR_ORDERED_DISCONNECT:
      return "Locally ordered disconnect";

    case DR_PROTOCOL_ERROR:
      return "Protocol error";

    case DR_SWITCH_PEER:
      return "Switch to higher priority peer";

    case DR_BAD_STATE:
      return "Connection in bad state";
  }

  return "Unknown disconnect reason";

} /* TcpConnection::disconnectReasonStr */


TcpConnection::TcpConnection(size_t recv_buf_len)
  : TcpConnection(-1, IpAddress(), 0, recv_buf_len)
{
} /* TcpConnection::TcpConnection */


TcpConnection::TcpConnection(int sock, const IpAddress& remote_addr,
      	      	      	     uint16_t remote_port, size_t recv_buf_len)
  : remote_addr(remote_addr), remote_port(remote_port), sock(sock)
{
  m_recv_buf.reserve(recv_buf_len);
  rd_watch.activity.connect(
      mem_fun(*this, &TcpConnection::recvHandler));
  m_wr_watch.activity.connect(
      mem_fun(*this, &TcpConnection::onWriteSpaceAvailable));
  setSocket(sock);
} /* TcpConnection::TcpConnection */


TcpConnection::~TcpConnection(void)
{
  closeConnection();
} /* TcpConnection::~TcpConnection */


TcpConnection& TcpConnection::operator=(TcpConnection&& other)
{
  //std::cout << "### TcpConnection::operator=(TcpConnection&&)" << std::endl;

  closeConnection();

  remote_addr = other.remote_addr;
  other.remote_addr.clear();

  remote_port = other.remote_port;
  other.remote_port = 0;

  sock = other.sock;
  other.sock = -1;

  rd_watch = std::move(other.rd_watch);
  m_wr_watch = std::move(other.m_wr_watch);

  m_recv_buf = std::move(other.m_recv_buf);
  other.m_recv_buf.clear();
  other.m_recv_buf.reserve(m_recv_buf.capacity());

  m_write_buf = std::move(other.m_write_buf);
  other.m_write_buf.clear();
  other.m_write_buf.reserve(m_write_buf.capacity());

  m_ssl_ctx = other.m_ssl_ctx;
  other.m_ssl_ctx = nullptr;

  m_ssl_is_server = other.m_ssl_is_server;
  other.m_ssl_is_server = false;

  m_ssl = other.m_ssl;
  other.m_ssl = nullptr;

  m_ssl_rd_bio = other.m_ssl_rd_bio;
  other.m_ssl_rd_bio = nullptr;

  m_ssl_wr_bio = other.m_ssl_wr_bio;
  other.m_ssl_wr_bio = nullptr;

  m_ssl_encrypt_buf = std::move(other.m_ssl_encrypt_buf);
  other.m_ssl_encrypt_buf.clear();
  other.m_ssl_encrypt_buf.reserve(m_ssl_encrypt_buf.capacity());

  return *this;
} /* TcpConnection::operator= */


void TcpConnection::setRecvBufLen(size_t recv_buf_len)
{
  if (recv_buf_len > m_recv_buf.size())
  {
    m_recv_buf.reserve(recv_buf_len);
  }
} /* TcpConnection::setRecvBufLen */


int TcpConnection::write(const void *buf, int count)
{
  assert(sock >= 0);
  if (m_ssl != nullptr)
  {
    return sslWrite(reinterpret_cast<const char*>(buf), count);
  }
  addToWriteBuf(reinterpret_cast<const char*>(buf), count);
  return count;
} /* TcpConnection::write */


void TcpConnection::enableSsl(bool enable)
{
  if (enable)
  {
    assert(m_ssl_ctx != nullptr);

    m_ssl_rd_bio = BIO_new(BIO_s_mem());
    m_ssl_wr_bio = BIO_new(BIO_s_mem());
    m_ssl = SSL_new(*m_ssl_ctx);
    ssl_con_map[m_ssl] = this;

    SSL_set_bio(m_ssl, m_ssl_rd_bio, m_ssl_wr_bio);

    if (m_ssl_is_server)
    {
      SSL_set_accept_state(m_ssl);
    }
    else
    {
      //SSL_set_tlsext_host_name(m_ssl, "svxreflector.example.com");
      SSL_set_connect_state(m_ssl);
      auto ret = sslDoHandshake();
      assert(ret != SSLSTATUS_FAIL);
    }

    //auto data_index = SSL_get_ex_new_index(0, NULL, NULL, NULL, NULL);
    //assert (data_index != -1);
    //SSL_set_ex_data(m_ssl, data_index, this);

    SSL_set_verify(m_ssl, SSL_VERIFY_PEER, sslVerifyCallback);
  }
  else
  {
    // FIXME: Deinitialize
  }
} /* TcpConnection::enableSsl */


void TcpConnection::setSslContext(SslContext& ctx, bool is_server)
{
  m_ssl_ctx = &ctx;
  m_ssl_is_server = is_server;
} /* TcpConnection::setSslContext */


Async::SslX509 TcpConnection::sslPeerCertificate(void)
{
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
  return Async::SslX509(SSL_get1_peer_certificate(m_ssl), true);
#else
  return Async::SslX509(SSL_get_peer_certificate(m_ssl), true);
#endif
} /* TcpConnection::sslPeerCertificate */


Async::SslX509 TcpConnection::sslCertificate(void) const
{
  auto x509 = SSL_CTX_get0_certificate(*m_ssl_ctx);
  return Async::SslX509(x509, false);
} /* TcpConnection::sslCertificate */


long TcpConnection::sslVerifyResult(void) const
{
  return SSL_get_verify_result(m_ssl);
} /* TcpConnection::sslVerifyResult */


#if 0
std::string TcpConnection::sslCommonName(void) const
{
  std::string cn;

  assert(m_ssl != nullptr);

  Async::SslX509 cert(SSL_get_peer_certificate(m_ssl));
  if(cert == nullptr)
  {
    std::cout << "### No certificate for this connection" << std::endl;
    return cn;
  }

  if (SSL_get_verify_result(m_ssl) == X509_V_OK)
  {
    X509_NAME* subj = cert.getSubjectName();

      // Assume there is only one CN
    int lastpos = X509_NAME_get_index_by_NID(subj, NID_commonName, -1);
    if (lastpos >= 0)
    {
      X509_NAME_ENTRY *e = X509_NAME_get_entry(subj, lastpos);
      ASN1_STRING *d = X509_NAME_ENTRY_get_data(e);
      cn = reinterpret_cast<const char*>(ASN1_STRING_get0_data(d));
      //std::cout << "### CN=" << cn << std::endl;
    }
  }
  else
  {
    std::cout << "### The certificate did not verify" << std::endl;
  }

  return cn;
} /* TcpConnection::sslCommonName */
#endif


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void TcpConnection::setSocket(int sock)
{
  this->sock = sock;
  rd_watch.setFd(sock, FdWatch::FD_WATCH_RD);
  rd_watch.setEnabled(sock >= 0);
  m_wr_watch.setEnabled(false);
  m_wr_watch.setFd(sock, FdWatch::FD_WATCH_WR);
} /* TcpConnection::setSocket */


void TcpConnection::setRemoteAddr(const IpAddress& remote_addr)
{
  this->remote_addr = remote_addr;
} /* TcpConnection::setRemoteAddr */


void TcpConnection::setRemotePort(uint16_t remote_port)
{
  this->remote_port = remote_port;
} /* TcpConnection::setRemotePort */


uint16_t TcpConnection::localPort(void) const
{
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  int ret = getsockname(sock, reinterpret_cast<struct sockaddr *>(&addr),
                        &len);
  if ((ret != 0) || (len != sizeof(addr)))
  {
    perror("getsockname");
    return 0;
  }
  //std::cout << "### TcpConnection::localPort: sin_port="
  //          << ntohs(addr.sin_port) << std::endl;
  return ntohs(addr.sin_port);
} /* TcpConnection::localPort */


Async::IpAddress TcpConnection::localHost(void) const
{
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  int ret = getsockname(sock, reinterpret_cast<struct sockaddr *>(&addr),
                        &len);
  if ((ret != 0) || (len != sizeof(addr)))
  {
    perror("getsockname");
    return Async::IpAddress();
  }
  //std::cout << "### TcpConnection::localHost: sin_addr="
  //          << Async::IpAddress(addr.sin_addr) << std::endl;
  return Async::IpAddress(addr.sin_addr);
} /* TcpConnection::localHost */


void TcpConnection::closeConnection(void)
{
  m_recv_buf.clear();
  m_write_buf.clear();
  m_ssl_encrypt_buf.clear();

  m_wr_watch.setEnabled(false);
  rd_watch.setEnabled(false);

  if (m_ssl != nullptr)
  {
    ssl_con_map.erase(m_ssl);
    SSL_free(m_ssl);
    m_ssl = nullptr;
  }

  if (sock >= 0)
  {
    close(sock);
    sock = -1;
  }
} /* TcpConnection::closeConnection */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

int TcpConnection::sslVerifyCallback(int preverify_ok,
                                     X509_STORE_CTX* x509_store_ctx)
{
  //std::cout << "### TcpConnection::sslVerifyCallback: preverify_ok: "
  //          << preverify_ok << std::endl;

  SSL* ssl = reinterpret_cast<SSL*>(X509_STORE_CTX_get_ex_data(x509_store_ctx,
      SSL_get_ex_data_X509_STORE_CTX_idx()));
  assert(ssl != nullptr);

  TcpConnection* con = lookupConnection(ssl);
  assert(con != nullptr);

  return con->emitVerifyPeer(preverify_ok, x509_store_ctx);
} /* TcpConnection::sslVerifyCallback */


void TcpConnection::recvHandler(FdWatch *watch)
{
  //std::cout << "### TcpConnection::recvHandler:"
  //          << " m_recv_buf.size()=" << m_recv_buf.size()
  //          << " m_recv_buf.capacity()=" << m_recv_buf.capacity()
  //          << std::endl;

  size_t recv_buf_size = m_recv_buf.size();
  int cnt = read(sock, &m_recv_buf[recv_buf_size],
                 m_recv_buf.capacity()-recv_buf_size);
  //std::cout << "###   cnt=" << cnt << std::endl;
  if (cnt <= -1)
  {
    int errno_tmp = errno;
    closeConnection();
    errno = errno_tmp;
    onDisconnected(DR_SYSTEM_ERROR);
    return;
  }
  else if (cnt == 0)
  {
    //cout << "Connection closed by remote host!\n";
    closeConnection();
    onDisconnected(DR_REMOTE_DISCONNECTED);
    return;
  }

  m_recv_buf.resize(recv_buf_size + cnt);
  //std::cout << "### TcpConnection::recvHandler: size="
  //          << m_recv_buf.size() << std::endl;
  if (m_recv_buf.size() == m_recv_buf.capacity())
  {
    size_t new_capacity = m_recv_buf.capacity() * 2;
    //std::cout << "### new_capacity=" << new_capacity << std::endl;
    m_recv_buf.reserve(new_capacity);
  }

  ssize_t processed = -1;
  if (m_ssl != nullptr)
  {
    processed = sslRecvHandler(reinterpret_cast<char*>(m_recv_buf.data()),
                               m_recv_buf.size());
  }
  else
  {
    processed = onDataReceived(reinterpret_cast<char*>(m_recv_buf.data()),
                               m_recv_buf.size());
  }
  //cout << "### processed=" << processed << endl;
  if (processed >= static_cast<ssize_t>(m_recv_buf.size()))
  {
    m_recv_buf.clear();
  }
  else if (processed > 0)
  {
    std::rotate(m_recv_buf.begin(), m_recv_buf.begin()+processed,
                m_recv_buf.end());
    m_recv_buf.resize(m_recv_buf.size() - processed);
  }
  else if (processed < 0)
  {
    std::cerr << "*** ERROR: Network communication failed with "
              << remoteHost() << ":" << remotePort()
              << std::endl;
    if (isConnected())
    {
      closeConnection();
      onDisconnected(DR_PROTOCOL_ERROR);
    }
  }
} /* TcpConnection::recvHandler */


void TcpConnection::addToWriteBuf(const char *buf, size_t len)
{
  m_write_buf.insert(m_write_buf.end(), buf, buf+len);
  m_wr_watch.setEnabled(!m_write_buf.empty());
} /* TcpConnection::addToWriteBuf */


void TcpConnection::onWriteSpaceAvailable(Async::FdWatch* w)
{
  ssize_t n = rawWrite(m_write_buf.data(), m_write_buf.size());
  //std::cout << "### TcpConnection::onWriteSpaceAvailabe:"
  //          << "  fd=" << w->fd()
  //          << "  n=" << n
  //          << "  bufsize=" << m_write_buf.size()
  //          << std::endl;
  assert(n <= static_cast<ssize_t>(m_write_buf.size()));
  if (n >= 0)
  {
    if (n == static_cast<ssize_t>(m_write_buf.size()))
    {
      m_write_buf.clear();
    }
    else if (n > 0)
    {
      std::rotate(m_write_buf.begin(), m_write_buf.begin() + n,
                  m_write_buf.end());
      m_write_buf.resize(m_write_buf.size() - n);
    }
  }
  else
  {
    perror("### TcpConnection::onWriteSpaceAvailable: rawWrite()");
  }
  w->setEnabled(!m_write_buf.empty());
} /* TcpConnection::onWriteSpaceAvailable */


int TcpConnection::rawWrite(const void* buf, int count)
{
  assert(sock != -1);
  int cnt = ::send(sock, buf, count, MSG_NOSIGNAL);
  if (cnt < 0)
  {
    if (errno != EAGAIN)
    {
      return -1;
    }
    cnt = 0;
  }

  return cnt;
} /* TcpConnection::rawWrite */


void TcpConnection::sslPrintErrors(const char* fname)
{
  std::cerr << "*** ERROR: OpenSSL \"" << fname << "\" failed: ";
  ERR_print_errors_fp(stderr);
} /* TcpConnection::sslPrintErrors */


TcpConnection::SslStatus TcpConnection::sslGetStatus(int n)
{
  int err = SSL_get_error(m_ssl, n);
  switch (err)
  {
    case SSL_ERROR_NONE:
      return SSLSTATUS_OK;
    case SSL_ERROR_WANT_WRITE:
    case SSL_ERROR_WANT_READ:
      return SSLSTATUS_WANT_IO;
    case SSL_ERROR_ZERO_RETURN:
    case SSL_ERROR_SYSCALL:
    default:
      return SSLSTATUS_FAIL;
  }
} /* TcpConnection::sslGetStatus */


int TcpConnection::sslRecvHandler(char* src, int count)
{
  //std::cout << "### TcpConnection::sslRecvHandler: count=" << count
  //          << std::endl;

  SslStatus status;
  int n;

  int orig_count = count;

  while (count > 0)
  {
    n = BIO_write(m_ssl_rd_bio, src, count);
    //std::cout << "### BIO_write: n=" << n << std::endl;
    if (n <= 0)
    {
      sslPrintErrors("BIO_write");
      return 0;
    }

    src += n;
    count -= n;

    if (!SSL_is_init_finished(m_ssl))
    {
      if (sslDoHandshake() == SSLSTATUS_FAIL)
      {
        sslPrintErrors("sslDoHandshake");
        return -1;
      }
      if ((m_ssl == nullptr) || !SSL_is_init_finished(m_ssl))
      {
        //std::cout << "### onDataReceived: init not finished" << std::endl;
        return (orig_count - count);
      }
    }

    /* The encrypted data is now in the input bio so now we can perform actual
     * read of unencrypted data. */
    char buf[DEFAULT_BUF_SIZE];
    //while (SSL_pending(m_ssl) > 0)
    do
    {
      if (m_ssl == nullptr)
      {
        return (orig_count - count);
      }
      n = SSL_read(m_ssl, buf, sizeof(buf));
      //std::cout << "### SSL_read: n=" << n << std::endl;
      if (n > 0)
      {
        onDataReceived(buf, n);
      }
    } while (n > 0);

    status = sslGetStatus(n);

    if (status == SSLSTATUS_FAIL)
    {
      sslPrintErrors("SSL_read/SSL_pending");
      return -1;
    }

    /* Did SSL request to write bytes? This can happen if peer has requested SSL
     * renegotiation. */
    if (status == SSLSTATUS_WANT_IO)
    {
      do {
        n = BIO_read(m_ssl_wr_bio, buf, sizeof(buf));
        if (n > 0)
        {
          addToWriteBuf(buf, n);
        }
        else if (!BIO_should_retry(m_ssl_wr_bio))
        {
          sslPrintErrors("BIO_should_retry");
          return -1;
        }
      } while (n > 0);
    }
  }

  return (orig_count - count);
} /* TcpConnection::readHandler */


enum TcpConnection::SslStatus TcpConnection::sslDoHandshake(void)
{
  char buf[DEFAULT_BUF_SIZE];
  SslStatus status;

  int n = SSL_do_handshake(m_ssl);
  status = sslGetStatus(n);

  /* Did SSL request to write bytes? */
  if (status == SSLSTATUS_WANT_IO)
  {
    do {
      n = BIO_read(m_ssl_wr_bio, buf, sizeof(buf));
      if (n > 0)
      {
        addToWriteBuf(buf, n);
      }
      else if (!BIO_should_retry(m_ssl_wr_bio))
        return SSLSTATUS_FAIL;
    } while (n>0);
  }

  if (SSL_is_init_finished(m_ssl))
  {
    sslConnectionReady(this);
    sslEncrypt();
  }

  return status;
} /* TcpConnection::sslDoHandshake */


int TcpConnection::sslEncrypt(void)
{
  char buf[DEFAULT_BUF_SIZE];
  SslStatus status;

  if ((m_ssl == nullptr) || !SSL_is_init_finished(m_ssl))
  {
    return 0;
  }

  while (!m_ssl_encrypt_buf.empty())
  {
    int n = SSL_write(m_ssl, m_ssl_encrypt_buf.data(),
                      m_ssl_encrypt_buf.size());
    status = sslGetStatus(n);

    if (n > 0)
    {
      if (n == static_cast<int>(m_ssl_encrypt_buf.size()))
      {
        m_ssl_encrypt_buf.clear();
      }
      else
      {
        std::rotate(m_ssl_encrypt_buf.begin(), m_ssl_encrypt_buf.begin()+n,
                    m_ssl_encrypt_buf.end());
        m_ssl_encrypt_buf.resize(m_ssl_encrypt_buf.size() - n);
      }

      /* take the output of the SSL object and queue it for socket write */
      do {
        n = BIO_read(m_ssl_wr_bio, buf, sizeof(buf));
        if (n > 0)
        {
          addToWriteBuf(buf, n);
        }
        else if (!BIO_should_retry(m_ssl_wr_bio))
        {
          return -1;
        }
      } while (n > 0);
    }

    if (status == SSLSTATUS_FAIL)
    {
      return -1;
    }

    if (n==0)
    {
      break;
    }
  }
  return 0;
} /* TcpConnection::sslEncrypt */


int TcpConnection::sslWrite(const void* buf, int count)
{
  const char* ptr = reinterpret_cast<const char*>(buf);
  m_ssl_encrypt_buf.insert(m_ssl_encrypt_buf.end(), ptr, ptr+count);
  sslEncrypt();
  return count;
} /* TcpConnection::sslWrite */


/*
 * This file has not been truncated
 */
