/**
@file	 Reflector.cpp
@brief   The main reflector class
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-11

\verbatim
SvxReflector - An audio reflector for connecting SvxLink Servers
Copyright (C) 2003-2024 Tobias Blomberg / SM0SVX

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

#include <cassert>
#include <json/json.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <regex>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncTcpServer.h>
#include <AsyncDigest.h>
#include <AsyncSslCertSigningReq.h>
#include <AsyncEncryptedUdpSocket.h>
#include <AsyncApplication.h>
#include <AsyncPty.h>

#include <common.h>
#include <config.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Reflector.h"
#include "ReflectorClient.h"
#include "TGHandler.h"


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

#define RENEW_AFTER 2/3


/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local functions
 *
 ****************************************************************************/

namespace {
  //void splitFilename(const std::string& filename, std::string& dirname,
  //    std::string& basename)
  //{
  //  std::string ext;
  //  basename = filename;

  //  size_t basenamepos = filename.find_last_of('/');
  //  if (basenamepos != string::npos)
  //  {
  //    if (basenamepos + 1 < filename.size())
  //    {
  //      basename = filename.substr(basenamepos + 1);
  //    }
  //    dirname = filename.substr(0, basenamepos + 1);
  //  }

  //  size_t extpos = basename.find_last_of('.');
  //  if (extpos != string::npos)
  //  {
  //    if (extpos+1 < basename.size())
  //    ext = basename.substr(extpos+1);
  //    basename.erase(extpos);
  //  }
  //}

  bool ensureDirectoryExist(const std::string& path)
  {
    std::vector<std::string> parts;
    SvxLink::splitStr(parts, path, "/");
    std::string dirname;
    if (path[0] == '/')
    {
      dirname = "/";
    }
    else if (path[0] != '.')
    {
      dirname = "./";
    }
    if (path.back() != '/')
    {
      parts.erase(std::prev(parts.end()));
    }
    for (const auto& part : parts)
    {
      dirname += part + "/";
      if (access(dirname.c_str(), F_OK) != 0)
      {
        std::cout << "Create directory '" << dirname << "'" << std::endl;
        if (mkdir(dirname.c_str(), 0755) != 0)
        {
          std::cerr << "*** ERROR: Could not create directory '"
                    << dirname << "'" << std::endl;
          return false;
        }
      }
    }
    return true;
  } /* ensureDirectoryExist */


  void startCertRenewTimer(const Async::SslX509& cert, Async::AtTimer& timer)
  {
    int days=0, seconds=0;
    cert.timeSpan(days, seconds);
    time_t renew_time = cert.notBefore() +
        (static_cast<time_t>(days)*24*3600 + seconds)*RENEW_AFTER;
    timer.setTimeout(renew_time);
    timer.setExpireOffset(10000);
    timer.start();
  } /* startCertRenewTimer */
};


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

namespace {
  ReflectorClient::ProtoVerRangeFilter v1_client_filter(
      ProtoVer(1, 0), ProtoVer(1, 999));
  //ReflectorClient::ProtoVerRangeFilter v2_client_filter(
  //    ProtoVer(2, 0), ProtoVer(2, 999));
  ReflectorClient::ProtoVerLargerOrEqualFilter ge_v2_client_filter(
      ProtoVer(2, 0));
};


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

Reflector::Reflector(void)
  : m_srv(0), m_udp_sock(0), m_tg_for_v1_clients(1), m_random_qsy_lo(0),
    m_random_qsy_hi(0), m_random_qsy_tg(0), m_http_server(0), m_cmd_pty(0),
    m_keys_dir("private/"), m_pending_csrs_dir("pending_csrs/"),
    m_csrs_dir("csrs/"), m_certs_dir("certs/"), m_pki_dir("pki/")
{
  TGHandler::instance()->talkerUpdated.connect(
      mem_fun(*this, &Reflector::onTalkerUpdated));
  TGHandler::instance()->requestAutoQsy.connect(
      mem_fun(*this, &Reflector::onRequestAutoQsy));
  m_renew_cert_timer.expired.connect(
      [&](Async::AtTimer*)
      {
        if (!loadServerCertificateFiles())
        {
          std::cerr << "*** WARNING: Failed to renew server certificate"
                    << std::endl;
        }
      });
  m_renew_issue_ca_cert_timer.expired.connect(
      [&](Async::AtTimer*)
      {
        if (!loadSigningCAFiles())
        {
          std::cerr << "*** WARNING: Failed to renew issuing CA certificate"
                    << std::endl;
        }
      });
} /* Reflector::Reflector */


Reflector::~Reflector(void)
{
  delete m_http_server;
  m_http_server = 0;
  delete m_udp_sock;
  m_udp_sock = 0;
  delete m_srv;
  m_srv = 0;
  delete m_cmd_pty;
  m_cmd_pty = 0;
  m_client_con_map.clear();
  ReflectorClient::cleanup();
  delete TGHandler::instance();
} /* Reflector::~Reflector */


bool Reflector::initialize(Async::Config &cfg)
{
  m_cfg = &cfg;
  TGHandler::instance()->setConfig(m_cfg);

  std::string listen_port("5300");
  cfg.getValue("GLOBAL", "LISTEN_PORT", listen_port);
  m_srv = new TcpServer<FramedTcpConnection>(listen_port);
  m_srv->clientConnected.connect(
      mem_fun(*this, &Reflector::clientConnected));
  m_srv->clientDisconnected.connect(
      mem_fun(*this, &Reflector::clientDisconnected));

  if (!loadCertificateFiles())
  {
    return false;
  }

  m_srv->setSslContext(m_ssl_ctx);

  uint16_t udp_listen_port = 5300;
  cfg.getValue("GLOBAL", "LISTEN_PORT", udp_listen_port);
  m_udp_sock = new Async::EncryptedUdpSocket(udp_listen_port);
  const char* err = "unknown reason";
  if ((err="bad allocation",          (m_udp_sock == 0)) ||
      (err="initialization failure",  !m_udp_sock->initOk()) ||
      (err="unsupported cipher",      !m_udp_sock->setCipher(UdpCipher::NAME)))
  {
    std::cerr << "*** ERROR: Could not initialize UDP socket due to "
              << err << std::endl;
    return false;
  }
  m_udp_sock->setCipherAADLength(UdpCipher::AADLEN);
  m_udp_sock->setTagLength(UdpCipher::TAGLEN);
  m_udp_sock->cipherDataReceived.connect(
      mem_fun(*this, &Reflector::udpCipherDataReceived));
  m_udp_sock->dataReceived.connect(
      mem_fun(*this, &Reflector::udpDatagramReceived));

  unsigned sql_timeout = 0;
  cfg.getValue("GLOBAL", "SQL_TIMEOUT", sql_timeout);
  TGHandler::instance()->setSqlTimeout(sql_timeout);

  unsigned sql_timeout_blocktime = 60;
  cfg.getValue("GLOBAL", "SQL_TIMEOUT_BLOCKTIME", sql_timeout_blocktime);
  TGHandler::instance()->setSqlTimeoutBlocktime(sql_timeout_blocktime);

  m_cfg->getValue("GLOBAL", "TG_FOR_V1_CLIENTS", m_tg_for_v1_clients);

  SvxLink::SepPair<uint32_t, uint32_t> random_qsy_range;
  if (m_cfg->getValue("GLOBAL", "RANDOM_QSY_RANGE", random_qsy_range))
  {
    m_random_qsy_lo = random_qsy_range.first;
    m_random_qsy_hi = m_random_qsy_lo + random_qsy_range.second-1;
    if ((m_random_qsy_lo < 1) || (m_random_qsy_hi < m_random_qsy_lo))
    {
      cout << "*** WARNING: Illegal RANDOM_QSY_RANGE specified. Ignored."
           << endl;
      m_random_qsy_hi = m_random_qsy_lo = 0;
    }
    m_random_qsy_tg = m_random_qsy_hi;
  }

  std::string http_srv_port;
  if (m_cfg->getValue("GLOBAL", "HTTP_SRV_PORT", http_srv_port))
  {
    m_http_server = new Async::TcpServer<Async::HttpServerConnection>(http_srv_port);
    m_http_server->clientConnected.connect(
        sigc::mem_fun(*this, &Reflector::httpClientConnected));
    m_http_server->clientDisconnected.connect(
        sigc::mem_fun(*this, &Reflector::httpClientDisconnected));
  }

    // Path for command PTY
  string pty_path;
  m_cfg->getValue("GLOBAL", "COMMAND_PTY", pty_path);
  if (!pty_path.empty())
  {
    m_cmd_pty = new Pty(pty_path);
    if ((m_cmd_pty == nullptr) || !m_cmd_pty->open())
    {
      std::cerr << "*** ERROR: Could not open command PTY '" << pty_path
                << "' as specified in configuration variable "
                   "GLOBAL/COMMAND_PTY" << std::endl;
      return false;
    }
    m_cmd_pty->setLineBuffered(true);
    m_cmd_pty->dataReceived.connect(
        mem_fun(*this, &Reflector::ctrlPtyDataReceived));
  }

  m_cfg->valueUpdated.connect(sigc::mem_fun(*this, &Reflector::cfgUpdated));

  return true;
} /* Reflector::initialize */


void Reflector::nodeList(std::vector<std::string>& nodes) const
{
  nodes.clear();
  for (const auto& item : m_client_con_map)
  {
    const std::string& callsign = item.second->callsign();
    if (!callsign.empty())
    {
      nodes.push_back(callsign);
    }
  }
} /* Reflector::nodeList */


void Reflector::broadcastMsg(const ReflectorMsg& msg,
                             const ReflectorClient::Filter& filter)
{
  for (const auto& item : m_client_con_map)
  {
    ReflectorClient *client = item.second;
    if (filter(client) &&
        (client->conState() == ReflectorClient::STATE_CONNECTED))
    {
      client->sendMsg(msg);
    }
  }
} /* Reflector::broadcastMsg */


bool Reflector::sendUdpDatagram(ReflectorClient *client,
    const ReflectorUdpMsg& msg)
{
  if (client->protoVer() >= ProtoVer(3, 0))
  {
    ReflectorUdpMsg header(msg.type());
    ostringstream ss;
    assert(header.pack(ss) && msg.pack(ss));

    m_udp_sock->setCipherIV(client->udpCipherIV());
    m_udp_sock->setCipherKey(client->udpCipherKey());
    UdpCipher::AAD aad{client->udpCipherIVCntrNext()};
    std::stringstream aadss;
    if (!aad.pack(aadss))
    {
      std::cout << "*** WARNING: Packing associated data failed for UDP "
                   "datagram to " << client->remoteHost() << ":"
                << client->remotePort() << std::endl;
      return false;
    }
    return m_udp_sock->write(client->remoteHost(), client->remoteUdpPort(),
                             aadss.str().data(), aadss.str().size(),
                             ss.str().data(), ss.str().size());
  }
  else
  {
    ReflectorUdpMsgV2 header(msg.type(), client->clientId(),
        client->udpCipherIVCntrNext() & 0xffff);
    ostringstream ss;
    assert(header.pack(ss) && msg.pack(ss));
    return m_udp_sock->UdpSocket::write(
        client->remoteHost(), client->remoteUdpPort(),
        ss.str().data(), ss.str().size());
  }
} /* Reflector::sendUdpDatagram */


void Reflector::broadcastUdpMsg(const ReflectorUdpMsg& msg,
                                const ReflectorClient::Filter& filter)
{
  for (const auto& item : m_client_con_map)
  {
    ReflectorClient *client = item.second;
    if (filter(client) &&
        (client->conState() == ReflectorClient::STATE_CONNECTED))
    {
      client->sendUdpMsg(msg);
    }
  }
} /* Reflector::broadcastUdpMsg */


void Reflector::requestQsy(ReflectorClient *client, uint32_t tg)
{
  uint32_t current_tg = TGHandler::instance()->TGForClient(client);
  if (current_tg == 0)
  {
    std::cout << client->callsign()
              << ": Cannot request QSY from TG #0" << std::endl;
    return;
  }

  if (tg == 0)
  {
    tg = nextRandomQsyTg();
    if (tg == 0) { return; }
  }

  cout << client->callsign() << ": Requesting QSY from TG #"
       << current_tg << " to TG #" << tg << endl;

  broadcastMsg(MsgRequestQsy(tg),
      ReflectorClient::mkAndFilter(
        ge_v2_client_filter,
        ReflectorClient::TgFilter(current_tg)));
} /* Reflector::requestQsy */


Async::SslCertSigningReq
Reflector::loadClientPendingCsr(const std::string& callsign)
{
  Async::SslCertSigningReq csr;
  (void)csr.readPemFile(m_pending_csrs_dir + "/" + callsign + ".csr");
  return csr;
} /* Reflector::loadClientPendingCsr */


Async::SslCertSigningReq
Reflector::loadClientCsr(const std::string& callsign)
{
  Async::SslCertSigningReq csr;
  (void)csr.readPemFile(m_csrs_dir + "/" + callsign + ".csr");
  return csr;
} /* Reflector::loadClientPendingCsr */


bool Reflector::signClientCert(Async::SslX509& cert)
{
  //std::cout << "### Reflector::signClientCert" << std::endl;

  cert.setSerialNumber();
  cert.setIssuerName(m_issue_ca_cert.subjectName());
  time_t tnow = time(NULL);
  cert.setNotBefore(tnow);
  cert.setNotAfter(tnow + CERT_VALIDITY_TIME);
  auto cn = cert.commonName();
  if (!cert.sign(m_issue_ca_pkey))
  {
    std::cerr << "*** ERROR: Certificate signing failed for client "
              << cn << std::endl;
    return false;
  }
  auto crtfile = m_certs_dir + "/" + cn + ".crt";
  if (cert.writePemFile(crtfile) && m_issue_ca_cert.appendPemFile(crtfile))
  {
    runCAHook({
        { "CA_OP",      "CSR_SIGNED" },
        { "CA_CRT_PEM", cert.pem() }
      });
  }
  else
  {
    std::cerr << "*** WARNING: Failed to write client certificate file '"
              << crtfile << "'" << std::endl;
  }
  return true;
} /* Reflector::signClientCert */


Async::SslX509 Reflector::signClientCsr(const std::string& cn)
{
  //std::cout << "### Reflector::signClientCsr" << std::endl;

  Async::SslX509 cert(nullptr);

  auto req = loadClientPendingCsr(cn);
  if (req.isNull())
  {
    std::cerr << "*** ERROR: Cannot find CSR to sign '" << req.filePath()
              << "'" << std::endl;
    return cert;
  }

  cert.clear();
  cert.setVersion(Async::SslX509::VERSION_3);
  cert.setSubjectName(req.subjectName());
  const Async::SslX509Extensions exts(req.extensions());
  Async::SslX509Extensions cert_exts;
  cert_exts.addBasicConstraints("critical, CA:FALSE");
  cert_exts.addKeyUsage(
      "critical, digitalSignature, keyEncipherment, keyAgreement");
  cert_exts.addExtKeyUsage("clientAuth");
  Async::SslX509ExtSubjectAltName san(exts.subjectAltName());
  cert_exts.addExtension(san);
  cert.addExtensions(cert_exts);
  Async::SslKeypair csr_pkey(req.publicKey());
  cert.setPublicKey(csr_pkey);

  if (!signClientCert(cert))
  {
    cert.set(nullptr);
  }

  std::string csr_path = m_csrs_dir + "/" + cn + ".csr";
  if (rename(req.filePath().c_str(), csr_path.c_str()) != 0)
  {
    char errstr[256];
    (void)strerror_r(errno, errstr, sizeof(errstr));
    std::cerr << "*** WARNING: Failed to move signed CSR from '"
              << req.filePath() << "' to '" << csr_path << "': "
              << errstr << std::endl;
  }

  auto client = ReflectorClient::lookup(cn);
  if ((client != nullptr) && !cert.isNull())
  {
    client->certificateUpdated(cert);
  }

  return cert;
} /* Reflector::signClientCsr */


Async::SslX509 Reflector::loadClientCertificate(const std::string& callsign)
{
  Async::SslX509 cert;
  if (!cert.readPemFile(m_certs_dir + "/" + callsign + ".crt") ||
      cert.isNull() ||
      !cert.verify(m_issue_ca_pkey) ||
      !cert.timeIsWithinRange())
  {
    return nullptr;
  }
  return cert;
} /* Reflector::loadClientCertificate */


std::string Reflector::clientCertPem(const std::string& callsign) const
{
  std::string crtfile(m_certs_dir + "/" + callsign + ".crt");
  std::ifstream ifs(crtfile);
  if (!ifs.good())
  {
    return std::string();
  }
  return std::string(std::istreambuf_iterator<char>{ifs}, {});
} /* Reflector::clientCertPem */


std::string Reflector::caBundlePem(void) const
{
  std::ifstream ifs(m_ca_bundle_file);
  if (ifs.good())
  {
    return std::string(std::istreambuf_iterator<char>{ifs}, {});
  }
  return std::string();
} /* Reflector::caBundlePem */


std::string Reflector::issuingCertPem(void) const
{
  return m_issue_ca_cert.pem();
} /* Reflector::issuingCertPem */


bool Reflector::callsignOk(const std::string& callsign) const
{
    // Empty check
  if (callsign.empty())
  {
    std::cout << "*** WARNING: The callsign is empty" << std::endl;
    return false;
  }

    // Accept check
  std::string accept_cs_re_str;
  if (!m_cfg->getValue("GLOBAL", "ACCEPT_CALLSIGN", accept_cs_re_str) ||
      accept_cs_re_str.empty())
  {
    accept_cs_re_str =
      "[A-Z0-9][A-Z]{0,2}\\d[A-Z0-9]{1,3}[A-Z](?:-[A-Z0-9]{1,3})?";
  }
  const std::regex accept_callsign_re(accept_cs_re_str);
  if (!std::regex_match(callsign, accept_callsign_re))
  {
    std::cerr << "*** WARNING: The callsign '" << callsign
              << "' is not accepted by configuration (ACCEPT_CALLSIGN)"
              << std::endl;
    return false;
  }

    // Reject check
  std::string reject_cs_re_str;
  m_cfg->getValue("GLOBAL", "REJECT_CALLSIGN", reject_cs_re_str);
  if (!reject_cs_re_str.empty())
  {
    const std::regex reject_callsign_re(reject_cs_re_str);
    if (std::regex_match(callsign, reject_callsign_re))
    {
      std::cerr << "*** WARNING: The callsign '" << callsign
                << "' has been rejected by configuration (REJECT_CALLSIGN)."
                << std::endl;
      return false;
    }
  }

  return true;
} /* Reflector::callsignOk */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void Reflector::clientConnected(Async::FramedTcpConnection *con)
{
  std::cout << con->remoteHost() << ":" << con->remotePort()
       << ": Client connected" << endl;
  ReflectorClient *client = new ReflectorClient(this, con, m_cfg);
  con->verifyPeer.connect(sigc::mem_fun(*this, &Reflector::onVerifyPeer));
  client->csrReceived.connect(
      sigc::mem_fun(*this, &Reflector::onCsrReceived));
  m_client_con_map[con] = client;
} /* Reflector::clientConnected */


void Reflector::clientDisconnected(Async::FramedTcpConnection *con,
                           Async::FramedTcpConnection::DisconnectReason reason)
{
  ReflectorClientConMap::iterator it = m_client_con_map.find(con);
  assert(it != m_client_con_map.end());
  ReflectorClient *client = (*it).second;

  TGHandler::instance()->removeClient(client);

  if (!client->callsign().empty())
  {
    cout << client->callsign() << ": ";
  }
  else
  {
    std::cout << con->remoteHost() << ":" << con->remotePort() << ": ";
  }
  std::cout << "Client disconnected: " << TcpConnection::disconnectReasonStr(reason)
       << endl;

  m_client_con_map.erase(it);

  if (!client->callsign().empty())
  {
    broadcastMsg(MsgNodeLeft(client->callsign()),
        ReflectorClient::ExceptFilter(client));
  }
  Application::app().runTask([=]{ delete client; });
} /* Reflector::clientDisconnected */


bool Reflector::udpCipherDataReceived(const IpAddress& addr, uint16_t port,
                                      void *buf, int count)
{
  if ((count <= 0) || (static_cast<size_t>(count) < UdpCipher::AADLEN))
  {
    std::cout << "### : Ignoring too short UDP datagram (" << count
              << " bytes)" << std::endl;
    return true;
  }

  stringstream ss;
  ss.write(reinterpret_cast<const char *>(buf), UdpCipher::AADLEN);
  assert(m_aad.unpack(ss));

  ReflectorClient* client = nullptr;
  if (m_aad.iv_cntr == 0)
  {
    UdpCipher::InitialAAD iaad;
    //std::cout << "### Reflector::udpCipherDataReceived: m_aad.iv_cntr="
    //          << m_aad.iv_cntr << std::endl;
    if (static_cast<size_t>(count) < iaad.packedSize())
    {
      std::cout << "### Reflector::udpCipherDataReceived: "
                   "Ignoring malformed UDP registration datagram" << std::endl;
      return true;
    }
    ss.clear();
    ss.write(reinterpret_cast<const char *>(buf)+UdpCipher::AADLEN,
        sizeof(UdpCipher::ClientId));

    Async::MsgPacker<UdpCipher::ClientId>::unpack(ss, iaad.client_id);
    //std::cout << "### Reflector::udpCipherDataReceived: client_id="
    //          << iaad.client_id << std::endl;
    auto client = ReflectorClient::lookup(iaad.client_id);
    if (client == nullptr)
    {
      std::cout << "### Could not find client id (" << iaad.client_id
                << ") specified in initial AAD datagram" << std::endl;
      return true;
    }
    m_udp_sock->setCipherIV(UdpCipher::IV{client->udpCipherIVRand(),
                                          client->clientId(), 0});
    m_udp_sock->setCipherKey(client->udpCipherKey());
    m_udp_sock->setCipherAADLength(iaad.packedSize());
  }
  else if ((client=ReflectorClient::lookup(std::make_pair(addr, port))))
  {
    //if (static_cast<size_t>(count) < UdpCipher::AADLEN)
    //{
    //  std::cout << "### Reflector::udpCipherDataReceived: Datagram too short "
    //               "to hold associated data" << std::endl;
    //  return true;
    //}

    //if (!aad_unpack_ok)
    //{
    //  std::cout << "*** WARNING: Unpacking associated data failed for UDP "
    //               "datagram from " << addr << ":" << port << std::endl;
    //  return true;
    //}
    //std::cout << "### Reflector::udpCipherDataReceived: m_aad.iv_cntr="
    //          << m_aad.iv_cntr << std::endl;
    m_udp_sock->setCipherIV(UdpCipher::IV{client->udpCipherIVRand(),
                                          client->clientId(), m_aad.iv_cntr});
    m_udp_sock->setCipherKey(client->udpCipherKey());
    m_udp_sock->setCipherAADLength(UdpCipher::AADLEN);
  }
  else
  {
    udpDatagramReceived(addr, port, nullptr, buf, count);
    return true;
  }

  return false;
} /* Reflector::udpCipherDataReceived */


void Reflector::udpDatagramReceived(const IpAddress& addr, uint16_t port,
                                    void* aadptr, void *buf, int count)
{
  //std::cout << "### Reflector::udpDatagramReceived:"
  //          << " addr=" << addr
  //          << " port=" << port
  //          << " count=" << count
  //          << std::endl;

  assert(m_udp_sock->cipherAADLength() >= UdpCipher::AADLEN);

  stringstream ss;
  ss.write(reinterpret_cast<const char *>(buf), static_cast<size_t>(count));

  ReflectorUdpMsg header;
  if (!header.unpack(ss))
  {
    cout << "*** WARNING: Unpacking message header failed for UDP datagram "
            "from " << addr << ":" << port << endl;
    return;
  }
  ReflectorUdpMsgV2 header_v2;

  ReflectorClient* client = nullptr;
  UdpCipher::AAD aad;
  if (aadptr != nullptr)
  {
    //std::cout << "### Reflector::udpDatagramReceived: m_aad.iv_cntr="
    //          << m_aad.iv_cntr << std::endl;

    stringstream aadss;
    aadss.write(reinterpret_cast<const char *>(aadptr),
        m_udp_sock->cipherAADLength());

    if (!aad.unpack(aadss))
    {
      return;
    }
    if (aad.iv_cntr == 0) // Client UDP registration
    {
      UdpCipher::InitialAAD iaad;
      assert(aadss.seekg(0));
      if (!iaad.unpack(aadss))
      {
        std::cout << "### Reflector::udpDatagramReceived: "
                     "Could not unpack iaad" << std::endl;
        return;
      }
      assert(iaad.iv_cntr == 0);
      //std::cout << "### Reflector::udpDatagramReceived: iaad.client_id="
      //          << iaad.client_id << std::endl;
      client = ReflectorClient::lookup(iaad.client_id);
      if (client == nullptr)
      {
        std::cout << "### Reflector::udpDatagramReceived: Could not find "
                     "client id " << iaad.client_id << std::endl;
        return;
      }
      else if (client->remoteUdpPort() == 0)
      {
        //client->setRemoteUdpPort(port);
      }
      else
      {
        std::cout << "### Reflector::udpDatagramReceived: Client "
                  << iaad.client_id << " already registered." << std::endl;
      }
      client->setUdpRxSeq(0);
      //client->sendUdpMsg(MsgUdpHeartbeat());
    }
    else
    {
      client = ReflectorClient::lookup(std::make_pair(addr, port));
      if (client == nullptr)
      {
        std::cout << "### Unknown client " << addr << ":" << port << std::endl;
        return;
      }
    }
  }
  else
  {
    ss.seekg(0);
    if (!header_v2.unpack(ss))
    {
      std::cout << "*** WARNING: Unpacking V2 message header failed for UDP "
              "datagram from " << addr << ":" << port << std::endl;
      return;
    }
    client = ReflectorClient::lookup(header_v2.clientId());
    if (client == nullptr)
    {
      std::cerr << "*** WARNING: Incoming V2 UDP datagram from " << addr << ":"
           << port << " has invalid client id " << header_v2.clientId()
           << std::endl;
      return;
    }
  }

  //auto client = ReflectorClient::lookup(std::make_pair(addr, port));
  //if (client == nullptr)
  //{
  //  client = ReflectorClient::lookup(header.clientId());
  //  if (client == nullptr)
  //  {
  //    cerr << "*** WARNING: Incoming UDP datagram from " << addr << ":" << port
  //         << " has invalid client id " << header.clientId() << endl;
  //    return;
  //  }
  //}

  if (addr != client->remoteHost())
  {
    cerr << "*** WARNING[" << client->callsign()
         << "]: Incoming UDP packet has the wrong source ip, "
         << addr << " instead of " << client->remoteHost() << endl;
    return;
  }
  if (client->remoteUdpPort() == 0)
  {
    client->setRemoteUdpPort(port);
    client->sendUdpMsg(MsgUdpHeartbeat());
  }
  if (port != client->remoteUdpPort())
  {
    cerr << "*** WARNING[" << client->callsign()
         << "]: Incoming UDP packet has the wrong source UDP "
            "port number, " << port << " instead of "
         << client->remoteUdpPort() << endl;
    return;
  }

    // Check sequence number
  if (client->protoVer() >= ProtoVer(3, 0))
  {
    if (aad.iv_cntr < client->nextUdpRxSeq()) // Frame out of sequence (ignore)
    {
      std::cout << client->callsign()
                << ": Dropping out of sequence UDP frame with seq="
                << aad.iv_cntr << std::endl;
      return;
    }
    else if (aad.iv_cntr > client->nextUdpRxSeq()) // Frame lost
    {
      std::cout << client->callsign() << ": UDP frame(s) lost. Expected seq="
                << client->nextUdpRxSeq()
                << " but received " << aad.iv_cntr
                << ". Resetting next expected sequence number to "
                << (aad.iv_cntr + 1) << std::endl;
    }
    client->setUdpRxSeq(aad.iv_cntr + 1);
  }
  else
  {
    uint16_t next_udp_rx_seq = client->nextUdpRxSeq() & 0xffff;
    uint16_t udp_rx_seq_diff = header_v2.sequenceNum() - next_udp_rx_seq;
    if (udp_rx_seq_diff > 0x7fff) // Frame out of sequence (ignore)
    {
      std::cout << client->callsign()
                << ": Dropping out of sequence frame with seq="
                << header_v2.sequenceNum() << ". Expected seq="
                << next_udp_rx_seq << std::endl;
      return;
    }
    else if (udp_rx_seq_diff > 0) // Frame(s) lost
    {
      cout << client->callsign()
           << ": UDP frame(s) lost. Expected seq=" << next_udp_rx_seq
           << ". Received seq=" << header_v2.sequenceNum() << endl;
    }
    client->setUdpRxSeq(header_v2.sequenceNum() + 1);
  }

  client->udpMsgReceived(header);

  //std::cout << "### Reflector::udpDatagramReceived: type="
  //          << header.type() << std::endl;
  switch (header.type())
  {
    case MsgUdpHeartbeat::TYPE:
      break;

    case MsgUdpAudio::TYPE:
    {
      if (!client->isBlocked())
      {
        MsgUdpAudio msg;
        if (!msg.unpack(ss))
        {
          cerr << "*** WARNING[" << client->callsign()
               << "]: Could not unpack incoming MsgUdpAudioV1 message" << endl;
          return;
        }
        uint32_t tg = TGHandler::instance()->TGForClient(client);
        if (!msg.audioData().empty() && (tg > 0))
        {
          ReflectorClient* talker = TGHandler::instance()->talkerForTG(tg);
          if (talker == 0)
          {
            TGHandler::instance()->setTalkerForTG(tg, client);
            talker = TGHandler::instance()->talkerForTG(tg);
          }
          if (talker == client)
          {
            TGHandler::instance()->setTalkerForTG(tg, client);
            broadcastUdpMsg(msg,
                ReflectorClient::mkAndFilter(
                  ReflectorClient::ExceptFilter(client),
                  ReflectorClient::TgFilter(tg)));
            //broadcastUdpMsgExcept(tg, client, msg,
            //    ProtoVerRange(ProtoVer(0, 6),
            //                  ProtoVer(1, ProtoVer::max().minor())));
            //MsgUdpAudio msg_v2(msg);
            //broadcastUdpMsgExcept(tg, client, msg_v2,
            //    ProtoVerRange(ProtoVer(2, 0), ProtoVer::max()));
          }
        }
      }
      break;
    }

    //case MsgUdpAudio::TYPE:
    //{
    //  if (!client->isBlocked())
    //  {
    //    MsgUdpAudio msg;
    //    if (!msg.unpack(ss))
    //    {
    //      cerr << "*** WARNING[" << client->callsign()
    //           << "]: Could not unpack incoming MsgUdpAudio message" << endl;
    //      return;
    //    }
    //    if (!msg.audioData().empty())
    //    {
    //      if (m_talker == 0)
    //      {
    //        setTalker(client);
    //        cout << m_talker->callsign() << ": Talker start on TG #"
    //             << msg.tg() << endl;
    //      }
    //      if (m_talker == client)
    //      {
    //        gettimeofday(&m_last_talker_timestamp, NULL);
    //        broadcastUdpMsgExcept(tg, client, msg,
    //            ProtoVerRange(ProtoVer(2, 0), ProtoVer::max()));
    //        MsgUdpAudioV1 msg_v1(msg.audioData());
    //        broadcastUdpMsgExcept(tg, client, msg_v1,
    //            ProtoVerRange(ProtoVer(0, 6),
    //                          ProtoVer(1, ProtoVer::max().minor())));
    //      }
    //    }
    //  }
    //  break;
    //}

    case MsgUdpFlushSamples::TYPE:
    {
      uint32_t tg = TGHandler::instance()->TGForClient(client);
      ReflectorClient* talker = TGHandler::instance()->talkerForTG(tg);
      if ((tg > 0) && (client == talker))
      {
        TGHandler::instance()->setTalkerForTG(tg, 0);
      }
        // To be 100% correct the reflector should wait for all connected
        // clients to send a MsgUdpAllSamplesFlushed message but that will
        // probably lead to problems, especially on reflectors with many
        // clients. We therefore acknowledge the flush immediately here to
        // the client who sent the flush request.
      client->sendUdpMsg(MsgUdpAllSamplesFlushed());
      break;
    }

    case MsgUdpAllSamplesFlushed::TYPE:
      // Ignore
      break;

    case MsgUdpSignalStrengthValues::TYPE:
    {
      if (!client->isBlocked())
      {
        MsgUdpSignalStrengthValues msg;
        if (!msg.unpack(ss))
        {
          cerr << "*** WARNING[" << client->callsign()
               << "]: Could not unpack incoming "
                  "MsgUdpSignalStrengthValues message" << endl;
          return;
        }
        typedef MsgUdpSignalStrengthValues::Rxs::const_iterator RxsIter;
        for (RxsIter it = msg.rxs().begin(); it != msg.rxs().end(); ++it)
        {
          const MsgUdpSignalStrengthValues::Rx& rx = *it;
          //std::cout << "### MsgUdpSignalStrengthValues:"
          //  << " id=" << rx.id()
          //  << " siglev=" << rx.siglev()
          //  << " enabled=" << rx.enabled()
          //  << " sql_open=" << rx.sqlOpen()
          //  << " active=" << rx.active()
          //  << std::endl;
          client->setRxSiglev(rx.id(), rx.siglev());
          client->setRxEnabled(rx.id(), rx.enabled());
          client->setRxSqlOpen(rx.id(), rx.sqlOpen());
          client->setRxActive(rx.id(), rx.active());
        }
      }
      break;
    }

    default:
      // Better ignoring unknown messages to make it easier to add messages to
      // the protocol but still be backwards compatible

      //cerr << "*** WARNING[" << client->callsign()
      //     << "]: Unknown UDP protocol message received: msg_type="
      //     << header.type() << endl;
      break;
  }
} /* Reflector::udpDatagramReceived */


void Reflector::onTalkerUpdated(uint32_t tg, ReflectorClient* old_talker,
                                ReflectorClient *new_talker)
{
  if (old_talker != 0)
  {
    cout << old_talker->callsign() << ": Talker stop on TG #" << tg << endl;
    broadcastMsg(MsgTalkerStop(tg, old_talker->callsign()),
        ReflectorClient::mkAndFilter(
          ge_v2_client_filter,
          ReflectorClient::mkOrFilter(
            ReflectorClient::TgFilter(tg),
            ReflectorClient::TgMonitorFilter(tg))));
    if (tg == tgForV1Clients())
    {
      broadcastMsg(MsgTalkerStopV1(old_talker->callsign()), v1_client_filter);
    }
    broadcastUdpMsg(MsgUdpFlushSamples(),
          ReflectorClient::mkAndFilter(
            ReflectorClient::TgFilter(tg),
            ReflectorClient::ExceptFilter(old_talker)));
  }
  if (new_talker != 0)
  {
    cout << new_talker->callsign() << ": Talker start on TG #" << tg << endl;
    broadcastMsg(MsgTalkerStart(tg, new_talker->callsign()),
        ReflectorClient::mkAndFilter(
          ge_v2_client_filter,
          ReflectorClient::mkOrFilter(
            ReflectorClient::TgFilter(tg),
            ReflectorClient::TgMonitorFilter(tg))));
    if (tg == tgForV1Clients())
    {
      broadcastMsg(MsgTalkerStartV1(new_talker->callsign()), v1_client_filter);
    }
  }
} /* Reflector::setTalker */


void Reflector::httpRequestReceived(Async::HttpServerConnection *con,
                                    Async::HttpServerConnection::Request& req)
{
  //std::cout << "### " << req.method << " " << req.target << std::endl;

  Async::HttpServerConnection::Response res;
  if ((req.method != "GET") && (req.method != "HEAD"))
  {
    res.setCode(501);
    res.setContent("application/json",
        "{\"msg\":\"" + req.method + ": Method not implemented\"}");
    con->write(res);
    return;
  }

  if (req.target != "/status")
  {
    res.setCode(404);
    res.setContent("application/json",
        "{\"msg\":\"Not found!\"}");
    con->write(res);
    return;
  }

  Json::Value status;
  status["nodes"] = Json::Value(Json::objectValue);
  for (const auto& item : m_client_con_map)
  {
    ReflectorClient* client = item.second;
    if (client->conState() != ReflectorClient::STATE_CONNECTED)
    {
      continue;
    }

    Json::Value node(client->nodeInfo());
    //node["addr"] = client->remoteHost().toString();
    node["protoVer"]["majorVer"] = client->protoVer().majorVer();
    node["protoVer"]["minorVer"] = client->protoVer().minorVer();
    auto tg = client->currentTG();
    if (!TGHandler::instance()->showActivity(tg))
    {
      tg = 0;
    }
    node["tg"] = tg;
    node["restrictedTG"] = TGHandler::instance()->isRestricted(tg);
    Json::Value tgs = Json::Value(Json::arrayValue);
    const std::set<uint32_t>& monitored_tgs = client->monitoredTGs();
    for (std::set<uint32_t>::const_iterator mtg_it=monitored_tgs.begin();
         mtg_it!=monitored_tgs.end(); ++mtg_it)
    {
      tgs.append(*mtg_it);
    }
    node["monitoredTGs"] = tgs;
    bool is_talker = TGHandler::instance()->talkerForTG(tg) == client;
    node["isTalker"] = is_talker;

    if (node.isMember("qth") && node["qth"].isArray())
    {
      //std::cout << "### Found qth" << std::endl;
      Json::Value& qths(node["qth"]);
      for (Json::Value::ArrayIndex i=0; i<qths.size(); ++i)
      {
        Json::Value& qth(qths[i]);
        if (qth.isMember("rx") && qth["rx"].isObject())
        {
          //std::cout << "### Found rx" << std::endl;
          Json::Value::Members rxs(qth["rx"].getMemberNames());
          for (Json::Value::Members::const_iterator it=rxs.begin(); it!=rxs.end(); ++it)
          {
            //std::cout << "### member=" << *it << std::endl;
            const std::string& rx_id_str(*it);
            if (rx_id_str.size() == 1)
            {
              char rx_id(rx_id_str[0]);
              Json::Value& rx(qth["rx"][rx_id_str]);
              if (client->rxExist(rx_id))
              {
                rx["siglev"] = client->rxSiglev(rx_id);
                rx["enabled"] = client->rxEnabled(rx_id);
                rx["sql_open"] = client->rxSqlOpen(rx_id);
                rx["active"] = client->rxActive(rx_id);
              }
            }
          }
        }
        if (qth.isMember("tx") && qth["tx"].isObject())
        {
          //std::cout << "### Found tx" << std::endl;
          Json::Value::Members txs(qth["tx"].getMemberNames());
          for (Json::Value::Members::const_iterator it=txs.begin(); it!=txs.end(); ++it)
          {
            //std::cout << "### member=" << *it << std::endl;
            const std::string& tx_id_str(*it);
            if (tx_id_str.size() == 1)
            {
              char tx_id(tx_id_str[0]);
              Json::Value& tx(qth["tx"][tx_id_str]);
              if (client->txExist(tx_id))
              {
                tx["transmit"] = client->txTransmit(tx_id);
              }
            }
          }
        }
      }
    }
    status["nodes"][client->callsign()] = node;
  }
  std::ostringstream os;
  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = ""; //The JSON document is written on a single line
  Json::StreamWriter* writer = builder.newStreamWriter();
  writer->write(status, &os);
  delete writer;
  res.setContent("application/json", os.str());
  if (req.method == "HEAD")
  {
    res.setSendContent(false);
  }
  res.setCode(200);
  con->write(res);
} /* Reflector::requestReceived */


void Reflector::httpClientConnected(Async::HttpServerConnection *con)
{
  //std::cout << "### HTTP Client connected: "
  //          << con->remoteHost() << ":" << con->remotePort() << std::endl;
  con->requestReceived.connect(sigc::mem_fun(*this, &Reflector::httpRequestReceived));
} /* Reflector::httpClientConnected */


void Reflector::httpClientDisconnected(Async::HttpServerConnection *con,
    Async::HttpServerConnection::DisconnectReason reason)
{
  //std::cout << "### HTTP Client disconnected: "
  //          << con->remoteHost() << ":" << con->remotePort()
  //          << ": " << Async::HttpServerConnection::disconnectReasonStr(reason)
  //          << std::endl;
} /* Reflector::httpClientDisconnected */


void Reflector::onRequestAutoQsy(uint32_t from_tg)
{
  uint32_t tg = nextRandomQsyTg();
  if (tg == 0) { return; }

  std::cout << "Requesting auto-QSY from TG #" << from_tg
            << " to TG #" << tg << std::endl;

  broadcastMsg(MsgRequestQsy(tg),
      ReflectorClient::mkAndFilter(
        ge_v2_client_filter,
        ReflectorClient::TgFilter(from_tg)));
} /* Reflector::onRequestAutoQsy */


uint32_t Reflector::nextRandomQsyTg(void)
{
  if (m_random_qsy_tg == 0)
  {
    std::cout << "*** WARNING: QSY request for random TG "
              << "requested but RANDOM_QSY_RANGE is empty" << std::endl;
    return 0;
  }

  assert (m_random_qsy_tg != 0);
  uint32_t range_size = m_random_qsy_hi-m_random_qsy_lo+1;
  uint32_t i;
  for (i=0; i<range_size; ++i)
  {
    m_random_qsy_tg = (m_random_qsy_tg < m_random_qsy_hi) ?
      m_random_qsy_tg+1 : m_random_qsy_lo;
    if (TGHandler::instance()->clientsForTG(m_random_qsy_tg).empty())
    {
      return m_random_qsy_tg;
    }
  }

  std::cout << "*** WARNING: No random TG available for QSY" << std::endl;
  return 0;
} /* Reflector::nextRandomQsyTg */


void Reflector::ctrlPtyDataReceived(const void *buf, size_t count)
{
  const char* ptr = reinterpret_cast<const char*>(buf);
  const std::string cmdline(ptr, ptr + count);
  //std::cout << "### Reflector::ctrlPtyDataReceived: " << cmdline
  //          << std::endl;
  std::istringstream ss(cmdline);
  std::ostringstream errss;
  std::string cmd;
  if (!(ss >> cmd))
  {
    errss << "Invalid PTY command '" << cmdline << "'";
    goto write_status;
  }
  std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

  if (cmd == "CFG")
  {
    std::string section, tag, value;
    if (!(ss >> section >> tag >> value) || !ss.eof())
    {
      errss << "Invalid CFG PTY command '" << cmdline << "'. "
               "Usage: CFG <section> <tag> <value>";
      goto write_status;
    }
    m_cfg->setValue(section, tag, value);
  }
  else if (cmd == "CA")
  {
    std::string subcmd;
    if (!(ss >> subcmd))
    {
      errss << "Invalid CA PTY command '" << cmdline << "'. "
               "Usage: CA PENDING|SIGN <callsign>|LS|RM <callsign>";
      goto write_status;
    }
    std::transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::toupper);
    if (subcmd == "SIGN")
    {
      std::string cn;
      if (!(ss >> cn))
      {
        errss << "Invalid CA SIGN PTY command '" << cmdline << "'. "
                 "Usage: CA SIGN <callsign>";
        goto write_status;
      }
      auto cert = signClientCsr(cn);
      if (!cert.isNull())
      {
        std::cout << "---------- Signed Client Certificate ----------"
                  << std::endl;
        cert.print(" ");
        std::cout << "-----------------------------------------------"
                  << std::endl;
      }
      else
      {
        errss << "Certificate signing failed";
      }
    }
    else if (subcmd == "RM")
    {
      std::string cn;
      if (!(ss >> cn))
      {
        errss << "Invalid CA RM PTY command '" << cmdline << "'. "
                 "Usage: CA RM <callsign>";
        goto write_status;
      }
      if (removeClientCert(cn))

      {
        std::cout << cn << ": Removed client certificate and CSR"
                  << std::endl;
      }
      else
      {
        errss << "Failed to remove certificate and CSR for '" << cn << "'";
      }
    }
    else if (subcmd == "LS")
    {
      errss << "Not yet implemented";
    }
    else if (subcmd == "PENDING")
    {
      errss << "Not yet implemented";
    }
    else
    {
      errss << "Invalid CA PTY command '" << cmdline << "'. "
               "Usage: CA PENDING|SIGN <callsign>|LS|RM <callsign>";
      goto write_status;
    }
  }
  else
  {
    errss << "Unknown PTY command '" << cmdline
          << "'. Valid commands are: CFG";
  }

  write_status:
    if (!errss.str().empty())
    {
      std::cerr << "*** ERROR: " << errss.str() << std::endl;
      m_cmd_pty->write(std::string("ERR:") + errss.str() + "\n");
      return;
    }
    m_cmd_pty->write("OK\n");
} /* Reflector::ctrlPtyDataReceived */


void Reflector::cfgUpdated(const std::string& section, const std::string& tag)
{
  std::string value;
  if (!m_cfg->getValue(section, tag, value))
  {
    std::cout << "*** ERROR: Failed to read updated configuration variable '"
              << section << "/" << tag << "'" << std::endl;
    return;
  }

  if (section == "GLOBAL")
  {
    if (tag == "SQL_TIMEOUT_BLOCKTIME")
    {
      unsigned t = TGHandler::instance()->sqlTimeoutBlocktime();
      if (!SvxLink::setValueFromString(t, value))
      {
        std::cout << "*** ERROR: Failed to set updated configuration "
                     "variable '" << section << "/" << tag << "'" << std::endl;
        return;
      }
      TGHandler::instance()->setSqlTimeoutBlocktime(t);
      //std::cout << "### New value for " << tag << "=" << t << std::endl;
    }
    else if (tag == "SQL_TIMEOUT")
    {
      unsigned t = TGHandler::instance()->sqlTimeout();
      if (!SvxLink::setValueFromString(t, value))
      {
        std::cout << "*** ERROR: Failed to set updated configuration "
                     "variable '" << section << "/" << tag << "'" << std::endl;
        return;
      }
      TGHandler::instance()->setSqlTimeout(t);
      //std::cout << "### New value for " << tag << "=" << t << std::endl;
    }
  }
} /* Reflector::cfgUpdated */


bool Reflector::loadCertificateFiles(void)
{
  if (!buildPath("GLOBAL", "CERT_PKI_DIR", SVX_LOCAL_STATE_DIR, m_pki_dir) ||
      !buildPath("GLOBAL", "CERT_CA_KEYS_DIR", m_pki_dir, m_keys_dir) ||
      !buildPath("GLOBAL", "CERT_CA_PENDING_CSRS_DIR", m_pki_dir,
                 m_pending_csrs_dir) ||
      !buildPath("GLOBAL", "CERT_CA_CSRS_DIR", m_pki_dir, m_csrs_dir) ||
      !buildPath("GLOBAL", "CERT_CA_CERTS_DIR", m_pki_dir, m_certs_dir))
  {
    return false;
  }

  if (!loadRootCAFiles() || !loadSigningCAFiles() ||
      !loadServerCertificateFiles())
  {
    return false;
  }

  if (!m_cfg->getValue("GLOBAL", "CERT_CA_BUNDLE", m_ca_bundle_file))
  {
    m_ca_bundle_file = m_pki_dir + "/ca-bundle.crt";
  }
  if (access(m_ca_bundle_file.c_str(), F_OK) != 0)
  {
    if (!ensureDirectoryExist(m_ca_bundle_file) ||
        !m_ca_cert.writePemFile(m_ca_bundle_file))
    {
      std::cout << "*** ERROR: Failed to write CA bundle file '"
                << m_ca_bundle_file << "'" << std::endl;
      return false;
    }
  }
  if (!m_ssl_ctx.setCaCertificateFile(m_ca_bundle_file))
  {
    std::cout << "*** ERROR: Failed to read CA certificate bundle '"
              << m_ca_bundle_file << "'" << std::endl;
    return false;
  }

  struct stat st;
  if (stat(m_ca_bundle_file.c_str(), &st) != 0)
  {
    char errstr[256];
    (void)strerror_r(errno, errstr, sizeof(errstr));
    std::cerr << "*** ERROR: Failed to read CA file from '"
              << m_ca_bundle_file << "': " << errstr << std::endl;
    return false;
  }
  auto bundle = caBundlePem();
  m_ca_size = bundle.size();
  Async::Digest ca_dgst;
  if (!ca_dgst.md(m_ca_md, MsgCABundle::MD_ALG, bundle))
  {
    std::cerr << "*** ERROR: CA bundle checksumming failed"
              << std::endl;
    return false;
  }
  ca_dgst.signInit(MsgCABundle::MD_ALG, m_issue_ca_pkey);
  m_ca_sig = ca_dgst.sign(bundle);
  m_ca_url = "";
  m_cfg->getValue("GLOBAL", "CERT_CA_URL", m_ca_url);

  return true;
} /* Reflector::loadCertificateFiles */


bool Reflector::loadServerCertificateFiles(void)
{
  std::string cert_cn;
  if (!m_cfg->getValue("SERVER_CERT", "COMMON_NAME", cert_cn) ||
      cert_cn.empty())
  {
    std::cerr << "*** ERROR: The 'SERVER_CERT/COMMON_NAME' variable is "
                 "unset which is needed for certificate signing request "
                 "generation." << std::endl;
    return false;
  }

  std::string keyfile;
  if (!m_cfg->getValue("SERVER_CERT", "KEYFILE", keyfile))
  {
    keyfile = m_keys_dir + "/" + cert_cn + ".key";
  }
  Async::SslKeypair pkey;
  if (access(keyfile.c_str(), F_OK) != 0)
  {
    std::cout << "Server private key file not found. Generating '"
              << keyfile << "'" << std::endl;
    if (!generateKeyFile(pkey, keyfile))
    {
      return false;
    }
  }
  else if (!pkey.readPrivateKeyFile(keyfile))
  {
    std::cerr << "*** ERROR: Failed to read private key file from '"
              << keyfile << "'" << std::endl;
    return false;
  }

  if (!m_cfg->getValue("SERVER_CERT", "CRTFILE", m_crtfile))
  {
    m_crtfile = m_certs_dir + "/" + cert_cn + ".crt";
  }
  Async::SslX509 cert;
  bool generate_cert = (access(m_crtfile.c_str(), F_OK) != 0);
  if (!generate_cert)
  {
    generate_cert = !cert.readPemFile(m_crtfile) ||
                    !cert.verify(m_issue_ca_pkey);
    if (generate_cert)
    {
      std::cerr << "*** WARNING: Failed to read server certificate "
                   "from '" << m_crtfile << "' or the cert is invalid. "
                   "Generating new certificate." << std::endl;
      cert.clear();
    }
    else
    {
      int days=0, seconds=0;
      cert.timeSpan(days, seconds);
      //std::cout << "### days=" << days << "  seconds=" << seconds
      //          << std::endl;
      time_t tnow = time(NULL);
      time_t renew_time = tnow + (days*24*3600 + seconds)*RENEW_AFTER;
      if (!cert.timeIsWithinRange(tnow, renew_time))
      {
        std::cerr << "Time to renew the server certificate '" << m_crtfile
                  << "'. It's valid until "
                  << cert.notAfterLocaltimeString() << "." << std::endl;
        cert.clear();
        generate_cert = true;
      }
    }
  }
  if (generate_cert)
  {
    //if (!pkey_fresh && !generateKeyFile(pkey, keyfile))
    //{
    //  return false;
    //}

    std::string csrfile;
    if (!m_cfg->getValue("SERVER_CERT", "CSRFILE", csrfile))
    {
      csrfile = m_csrs_dir + "/" + cert_cn + ".csr";
    }
    Async::SslCertSigningReq req;
    std::cout << "Generating server certificate signing request file '"
              << csrfile << "'" << std::endl;
    req.setVersion(Async::SslCertSigningReq::VERSION_1);
    req.addSubjectName("CN", cert_cn);
    Async::SslX509Extensions req_exts;
    req_exts.addBasicConstraints("critical, CA:FALSE");
    req_exts.addKeyUsage(
        "critical, digitalSignature, keyEncipherment, keyAgreement");
    req_exts.addExtKeyUsage("serverAuth");
    std::stringstream csr_san_ss;
    csr_san_ss << "DNS:" << cert_cn;
    std::string cert_san_str;
    if (m_cfg->getValue("SERVER_CERT", "SUBJECT_ALT_NAME", cert_san_str) &&
        !cert_san_str.empty())
    {
      csr_san_ss << "," << cert_san_str;
    }
    std::string email_address;
    if (m_cfg->getValue("SERVER_CERT", "EMAIL_ADDRESS", email_address) &&
        !email_address.empty())
    {
      csr_san_ss << ",email:" << email_address;
    }
    req_exts.addSubjectAltNames(csr_san_ss.str());
    req.addExtensions(req_exts);
    req.setPublicKey(pkey);
    req.sign(pkey);
    if (!req.writePemFile(csrfile))
    {
      // FIXME: Read SSL error stack

      std::cerr << "*** WARNING: Failed to write server certificate "
                   "signing request file to '" << csrfile << "'"
                << std::endl;
      //return false;
    }
    std::cout << "-------- Certificate Signing Request -------" << std::endl;
    req.print();
    std::cout << "--------------------------------------------" << std::endl;

    std::cout << "Generating server certificate file '" << m_crtfile << "'"
              << std::endl;
    cert.setSerialNumber();
    cert.setVersion(Async::SslX509::VERSION_3);
    cert.setIssuerName(m_issue_ca_cert.subjectName());
    cert.setSubjectName(req.subjectName());
    time_t tnow = time(NULL);
    cert.setNotBefore(tnow);
    cert.setNotAfter(tnow + CERT_VALIDITY_TIME);
    cert.addExtensions(req.extensions());
    cert.setPublicKey(pkey);
    cert.sign(m_issue_ca_pkey);
    assert(cert.verify(m_issue_ca_pkey));
    if (!ensureDirectoryExist(m_crtfile) || !cert.writePemFile(m_crtfile) ||
        !m_issue_ca_cert.appendPemFile(m_crtfile))
    {
      std::cout << "*** ERROR: Failed to write server certificate file '"
                << m_crtfile << "'" << std::endl;
      return false;
    }
  }
  std::cout << "------------ Server Certificate ------------" << std::endl;
  cert.print();
  std::cout << "--------------------------------------------" << std::endl;

  if (!m_ssl_ctx.setCertificateFiles(keyfile, m_crtfile))
  {
      std::cout << "*** ERROR: Failed to read and verify key ('"
                << keyfile << "') and certificate ('"
                << m_crtfile << "') files. "
                << "If key- and cert-file does not match, the certificate "
                   "is invalid for any other reason, you need "
                   "to remove the cert file in order to trigger the "
                   "generation of a new certificate signing request."
                   "Then the CSR need to be signed by the CA which creates a "
                   "valid certificate."
                << std::endl;
      return false;
  }

  startCertRenewTimer(cert, m_renew_cert_timer);

  return true;
} /* Reflector::loadServerCertificateFiles */


bool Reflector::generateKeyFile(Async::SslKeypair& pkey,
                                const std::string& keyfile)
{
  pkey.generate(2048);
  if (!ensureDirectoryExist(keyfile) || !pkey.writePrivateKeyFile(keyfile))
  {
    std::cerr << "*** ERROR: Failed to write private key file to '"
              << keyfile << "'" << std::endl;
    return false;
  }
  return true;
} /* Reflector::generateKeyFile */


bool Reflector::loadRootCAFiles(void)
{
    // Read root CA private key or generate a new one if it does not exist
  std::string ca_keyfile;
  if (!m_cfg->getValue("ROOT_CA", "KEYFILE", ca_keyfile))
  {
    ca_keyfile = m_keys_dir + "/svxreflector_root_ca.key";
  }
  if (access(ca_keyfile.c_str(), F_OK) != 0)
  {
    std::cout << "Root CA private key file not found. Generating '"
              << ca_keyfile << "'" << std::endl;
    if (!m_ca_pkey.generate(4096))
    {
      std::cout << "*** ERROR: Failed to generate root CA key" << std::endl;
      return false;
    }
    if (!ensureDirectoryExist(ca_keyfile) ||
        !m_ca_pkey.writePrivateKeyFile(ca_keyfile))
    {
      std::cerr << "*** ERROR: Failed to write root CA private key file to '"
                << ca_keyfile << "'" << std::endl;
      return false;
    }
  }
  else if (!m_ca_pkey.readPrivateKeyFile(ca_keyfile))
  {
    std::cerr << "*** ERROR: Failed to read root CA private key file from '"
              << ca_keyfile << "'" << std::endl;
    return false;
  }

    // Read the root CA certificate or generate a new one if it does not exist
  std::string ca_crtfile;
  if (!m_cfg->getValue("ROOT_CA", "CRTFILE", ca_crtfile))
  {
    ca_crtfile = m_certs_dir + "/svxreflector_root_ca.crt";
  }
  bool generate_ca_cert = (access(ca_crtfile.c_str(), F_OK) != 0);
  if (!generate_ca_cert)
  {
    if (!m_ca_cert.readPemFile(ca_crtfile) ||
        !m_ca_cert.verify(m_ca_pkey) ||
        !m_ca_cert.timeIsWithinRange())
    {
      std::cerr << "*** ERROR: Failed to read root CA certificate file "
                   "from '" << ca_crtfile << "' or the cert is invalid."
                << std::endl;
      return false;
    }
  }
  if (generate_ca_cert)
  {
    std::cout << "Generating root CA certificate file '" << ca_crtfile << "'"
              << std::endl;
    m_ca_cert.setSerialNumber();
    m_ca_cert.setVersion(Async::SslX509::VERSION_3);

    std::string value;
    value = "SvxReflector Root CA";
    (void)m_cfg->getValue("ROOT_CA", "COMMON_NAME", value);
    if (value.empty())
    {
      std::cerr << "*** ERROR: The 'ROOT_CA/COMMON_NAME' variable is "
                   "unset which is needed for root CA certificate generation."
                << std::endl;
      return false;
    }
    m_ca_cert.addIssuerName("CN", value);
    if (m_cfg->getValue("ROOT_CA", "ORG_UNIT", value) &&
        !value.empty())
    {
      m_ca_cert.addIssuerName("OU", value);
    }
    if (m_cfg->getValue("ROOT_CA", "ORG", value) && !value.empty())
    {
      m_ca_cert.addIssuerName("O", value);
    }
    if (m_cfg->getValue("ROOT_CA", "LOCALITY", value) &&
        !value.empty())
    {
      m_ca_cert.addIssuerName("L", value);
    }
    if (m_cfg->getValue("ROOT_CA", "STATE", value) && !value.empty())
    {
      m_ca_cert.addIssuerName("ST", value);
    }
    if (m_cfg->getValue("ROOT_CA", "COUNTRY", value) && !value.empty())
    {
      m_ca_cert.addIssuerName("C", value);
    }
    m_ca_cert.setSubjectName(m_ca_cert.issuerName());
    Async::SslX509Extensions ca_exts;
    ca_exts.addBasicConstraints("critical, CA:TRUE");
    ca_exts.addKeyUsage("critical, cRLSign, digitalSignature, keyCertSign");
    if (m_cfg->getValue("ROOT_CA", "EMAIL_ADDRESS", value) &&
        !value.empty())
    {
      ca_exts.addSubjectAltNames("email:" + value);
    }
    m_ca_cert.addExtensions(ca_exts);
    time_t tnow = time(NULL);
    m_ca_cert.setNotBefore(tnow);
    m_ca_cert.setNotAfter(tnow + 25*365*24*3600);
    m_ca_cert.setPublicKey(m_ca_pkey);
    m_ca_cert.sign(m_ca_pkey);
    if (!m_ca_cert.writePemFile(ca_crtfile))
    {
      std::cout << "*** ERROR: Failed to write root CA certificate file '"
                << ca_crtfile << "'" << std::endl;
      return false;
    }
  }
  std::cout << "----------- Root CA Certificate ------------" << std::endl;
  m_ca_cert.print();
  std::cout << "--------------------------------------------" << std::endl;

  return true;
} /* Reflector::loadRootCAFiles */


bool Reflector::loadSigningCAFiles(void)
{
    // Read issuing CA private key or generate a new one if it does not exist
  std::string ca_keyfile;
  if (!m_cfg->getValue("ISSUING_CA", "KEYFILE", ca_keyfile))
  {
    ca_keyfile = m_keys_dir + "/svxreflector_issuing_ca.key";
  }
  if (access(ca_keyfile.c_str(), F_OK) != 0)
  {
    std::cout << "Issuing CA private key file not found. Generating '"
              << ca_keyfile << "'" << std::endl;
    if (!m_issue_ca_pkey.generate(2048))
    {
      std::cout << "*** ERROR: Failed to generate CA key" << std::endl;
      return false;
    }
    if (!ensureDirectoryExist(ca_keyfile) ||
        !m_issue_ca_pkey.writePrivateKeyFile(ca_keyfile))
    {
      std::cerr << "*** ERROR: Failed to write issuing CA private key file "
                   "to '" << ca_keyfile << "'" << std::endl;
      return false;
    }
  }
  else if (!m_issue_ca_pkey.readPrivateKeyFile(ca_keyfile))
  {
    std::cerr << "*** ERROR: Failed to read issuing CA private key file "
                 "from '" << ca_keyfile << "'" << std::endl;
    return false;
  }

    // Read the CA certificate or generate a new one if it does not exist
  std::string ca_crtfile;
  if (!m_cfg->getValue("ISSUING_CA", "CRTFILE", ca_crtfile))
  {
    ca_crtfile = m_certs_dir + "/svxreflector_issuing_ca.crt";
  }
  bool generate_ca_cert = (access(ca_crtfile.c_str(), F_OK) != 0);
  if (!generate_ca_cert)
  {
    generate_ca_cert = !m_issue_ca_cert.readPemFile(ca_crtfile) ||
                       !m_issue_ca_cert.verify(m_ca_pkey) ||
                       !m_issue_ca_cert.timeIsWithinRange();
    if (generate_ca_cert)
    {
      std::cerr << "*** WARNING: Failed to read issuing CA certificate "
                   "from '" << ca_crtfile << "' or the cert is invalid. "
                   "Generating new certificate." << std::endl;
      m_issue_ca_cert.clear();
    }
    else
    {
      int days=0, seconds=0;
      m_issue_ca_cert.timeSpan(days, seconds);
      time_t tnow = time(NULL);
      time_t renew_time = tnow + (days*24*3600 + seconds)*RENEW_AFTER;
      if (!m_issue_ca_cert.timeIsWithinRange(tnow, renew_time))
      {
        std::cerr << "Time to renew the issuing CA certificate '"
                  << ca_crtfile << "'. It's valid until "
                  << m_issue_ca_cert.notAfterLocaltimeString() << "."
                  << std::endl;
        m_issue_ca_cert.clear();
        generate_ca_cert = true;
      }
    }
  }

  if (generate_ca_cert)
  {
    std::string ca_csrfile;
    if (!m_cfg->getValue("ISSUING_CA", "CSRFILE", ca_csrfile))
    {
      ca_csrfile = m_csrs_dir + "/svxreflector_issuing_ca.csr";
    }
    std::cout << "Generating issuing CA CSR file '" << ca_csrfile
              << "'" << std::endl;
    Async::SslCertSigningReq csr;
    csr.setVersion(Async::SslCertSigningReq::VERSION_1);
    std::string value;
    value = "SvxReflector Issuing CA";
    (void)m_cfg->getValue("ISSUING_CA", "COMMON_NAME", value);
    if (value.empty())
    {
      std::cerr << "*** ERROR: The 'ISSUING_CA/COMMON_NAME' variable is "
                   "unset which is needed for issuing CA certificate "
                   "generation." << std::endl;
      return false;
    }
    csr.addSubjectName("CN", value);
    if (m_cfg->getValue("ISSUING_CA", "ORG_UNIT", value) &&
        !value.empty())
    {
      csr.addSubjectName("OU", value);
    }
    if (m_cfg->getValue("ISSUING_CA", "ORG", value) && !value.empty())
    {
      csr.addSubjectName("O", value);
    }
    if (m_cfg->getValue("ISSUING_CA", "LOCALITY", value) && !value.empty())
    {
      csr.addSubjectName("L", value);
    }
    if (m_cfg->getValue("ISSUING_CA", "STATE", value) && !value.empty())
    {
      csr.addSubjectName("ST", value);
    }
    if (m_cfg->getValue("ISSUING_CA", "COUNTRY", value) && !value.empty())
    {
      csr.addSubjectName("C", value);
    }
    Async::SslX509Extensions exts;
    exts.addBasicConstraints("critical, CA:TRUE");
    exts.addKeyUsage("critical, cRLSign, digitalSignature, keyCertSign");
    if (m_cfg->getValue("ISSUING_CA", "EMAIL_ADDRESS", value) &&
        !value.empty())
    {
      exts.addSubjectAltNames("email:" + value);
    }
    csr.addExtensions(exts);
    csr.setPublicKey(m_issue_ca_pkey);
    csr.sign(m_issue_ca_pkey);
    //csr.print();
    if (!csr.writePemFile(ca_csrfile))
    {
      std::cout << "*** ERROR: Failed to write issuing CA CSR file '"
                << ca_csrfile << "'" << std::endl;
      return false;
    }

    std::cout << "Generating issuing CA certificate file '" << ca_crtfile
              << "'" << std::endl;
    m_issue_ca_cert.setSerialNumber();
    m_issue_ca_cert.setVersion(Async::SslX509::VERSION_3);
    m_issue_ca_cert.setSubjectName(csr.subjectName());
    m_issue_ca_cert.addExtensions(csr.extensions());
    time_t tnow = time(NULL);
    m_issue_ca_cert.setNotBefore(tnow);
    m_issue_ca_cert.setNotAfter(tnow + 4*CERT_VALIDITY_TIME);
    m_issue_ca_cert.setPublicKey(m_issue_ca_pkey);
    m_issue_ca_cert.setIssuerName(m_ca_cert.subjectName());
    m_issue_ca_cert.sign(m_ca_pkey);
    if (!m_issue_ca_cert.writePemFile(ca_crtfile))
    {
      std::cout << "*** ERROR: Failed to write issuing CA certificate file '"
                << ca_crtfile << "'" << std::endl;
      return false;
    }
  }
  std::cout << "---------- Issuing CA Certificate ----------" << std::endl;
  m_issue_ca_cert.print();
  std::cout << "--------------------------------------------" << std::endl;

  startCertRenewTimer(m_issue_ca_cert, m_renew_issue_ca_cert_timer);

  return true;
} /* Reflector::loadSigningCAFiles */


bool Reflector::onVerifyPeer(TcpConnection *con, bool preverify_ok,
                             X509_STORE_CTX *x509_store_ctx)
{
  //std::cout << "### Reflector::onVerifyPeer: preverify_ok="
  //          << (preverify_ok ? "yes" : "no") << std::endl;

  Async::SslX509 cert(*x509_store_ctx);
  preverify_ok = preverify_ok && !cert.isNull();
  preverify_ok = preverify_ok && !cert.commonName().empty();
  if (!preverify_ok)
  {
    std::cout << "*** ERROR: Certificate verification failed for client"
              << std::endl;
    std::cout << "------------ Client Certificate -------------" << std::endl;
    cert.print();
    std::cout << "---------------------------------------------" << std::endl;
  }

  return preverify_ok;
} /* Reflector::onVerifyPeer */


Async::SslX509 Reflector::onCsrReceived(Async::SslCertSigningReq& req)
{
  if (req.isNull())
  {
    return nullptr;
  }

  std::string callsign(req.commonName());
  if (!callsignOk(callsign))
  {
    std::cerr << "*** WARNING: The CSR CN (callsign) check failed"
              << std::endl;
    return nullptr;
  }

  std::string csr_path(m_csrs_dir + "/" + callsign + ".csr");
  Async::SslCertSigningReq csr;
  if (!csr.readPemFile(csr_path))
  {
    csr.set(nullptr);
  }

  if (!csr.isNull() && (req.publicKey() != csr.publicKey()))
  {
    std::cerr << "*** WARNING: The received CSR with callsign '"
              << callsign << "' has a different public key "
                 "than the current CSR. That may be a sign of someone "
                 "trying to highjack a callsign or the owner of the "
                 "callsign has generated a new private/public key pair."
              << std::endl;
    return nullptr;
  }

  std::string crtfile(m_certs_dir + "/" + callsign + ".crt");
  Async::SslX509 cert;
  if (!cert.readPemFile(crtfile) || !cert.verify(m_issue_ca_pkey) ||
      !cert.timeIsWithinRange() || (cert.publicKey() != req.publicKey()))
  {
    cert.set(nullptr);
  }

  const std::string pending_csr_path(
      m_pending_csrs_dir + "/" + callsign + ".csr");
  Async::SslCertSigningReq pending_csr;
  if ((
        csr.isNull() ||
        (req.digest() != csr.digest()) ||
        cert.isNull()
      ) && (
        !pending_csr.readPemFile(pending_csr_path) ||
        (req.digest() != pending_csr.digest())
      ))
  {
    std::cout << callsign << ": Add pending CSR '" << pending_csr_path
              << "' to CA" << std::endl;
    if (req.writePemFile(pending_csr_path))
    {
      const auto ca_op =
        pending_csr.isNull() ? "PENDING_CSR_CREATE" : "PENDING_CSR_UPDATE";
      runCAHook({
          { "CA_OP",      ca_op },
          { "CA_CSR_PEM", req.pem() }
        });
    }
    else
    {
      std::cerr << "*** WARNING: Could not write CSR file '"
                << pending_csr_path << "'" << std::endl;
    }
  }
  else
  {
    std::cout << callsign << ": The new CSR is the same as the already "
              << "existing CSR, so ignoring the new one" << std::endl;
  }

  return cert;
} /* Reflector::onCsrReceived */


bool Reflector::buildPath(const std::string& sec,    const std::string& tag,
                          const std::string& defdir, std::string& defpath)
{
  bool isdir = (defpath.back() == '/');
  std::string path(defpath);
  if (!m_cfg->getValue(sec, tag, path) || path.empty())
  {
    path = defpath;
  }
  //std::cout << "### sec=" << sec << "  tag=" << tag << "  defdir=" << defdir << "  defpath=" << defpath << "  path=" << path << std::endl;
  if ((path.front() != '/') && (path.front() != '.'))
  {
    path = defdir + "/" + defpath;
  }
  if (!ensureDirectoryExist(path))
  {
    return false;
  }
  if (isdir && (path.back() == '/'))
  {
    defpath = path.substr(0, path.size()-1);
  }
  else
  {
    defpath = std::move(path);
  }
  //std::cout << "### defpath=" << defpath << std::endl;
  return true;
} /* Reflector::buildPath */


bool Reflector::removeClientCert(const std::string& cn)
{
  std::cout << "### Reflector::removeClientCert: cn=" << cn << std::endl;
  return true;
} /* Reflector::removeClientCert */


void Reflector::runCAHook(const Async::Exec::Environment& env)
{
  auto ca_hook_cmd = m_cfg->getValue("GLOBAL", "CERT_CA_HOOK");
  if (!ca_hook_cmd.empty())
  {
    auto ca_hook = new Async::Exec(ca_hook_cmd);
    ca_hook->addEnvironmentVars(env);
    ca_hook->setTimeout(300); // Five minutes timeout
    ca_hook->stdoutData.connect(
        [=](const char* buf, int cnt)
        {
          std::cout << buf;
        });
    ca_hook->stderrData.connect(
        [=](const char* buf, int cnt)
        {
          std::cerr << buf;
        });
    ca_hook->exited.connect(
        [=](void) {
          if (ca_hook->ifExited())
          {
            if (ca_hook->exitStatus() != 0)
            {
              std::cerr << "*** ERROR: CA hook exited with exit status "
                        << ca_hook->exitStatus() << std::endl;
            }
          }
          else if (ca_hook->ifSignaled())
          {
            std::cerr << "*** ERROR: CA hook exited with signal "
                      << ca_hook->termSig() << std::endl;
          }
          Async::Application::app().runTask([=]{ delete ca_hook; });
        });
    ca_hook->run();
  }
} /* Reflector::runCAHook */


/*
 * This file has not been truncated
 */

