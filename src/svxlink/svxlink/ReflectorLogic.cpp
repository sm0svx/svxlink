/**
@file	 ReflectorLogic.cpp
@brief   A logic core that connect to the SvxReflector
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-12

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
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

#include <unistd.h>
//#include <openssl/x509.h>
//#include <openssl/x509v3.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <streambuf>
#include <limits>
#include <numeric>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncApplication.h>
#include <AsyncTcpClient.h>
#include <AsyncDigest.h>
#include <AsyncSslKeypair.h>
#include <AsyncSslCertSigningReq.h>
#include <AsyncEncryptedUdpSocket.h>
#include <AsyncIpAddress.h>
#include <AsyncAudioPassthrough.h>
#include <AsyncAudioValve.h>
#include <version/SVXLINK.h>
#include <config.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ReflectorLogic.h"
#include "EventHandler.h"


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

  template <class T>
  void hexdump(const T& d)
  {
    std::ostringstream ss;
    std::string sep(48, '-');
    ss << sep << "\n";
    ss << std::setfill('0') << std::hex;
    size_t cnt = 0;
    for (const auto& byte : d)
    {
      std::string spacer(" ");
      if (++cnt % 16 == 0)
      {
        spacer = "\n";
      }
      ss << std::setw(2) << unsigned(byte) << spacer;
    }
    std::cout << ss.str() << ((cnt % 16 > 0) ? "\n" : "")
              << sep << std::endl;
  }
};


/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global functions
 *
 ****************************************************************************/

extern "C" {
  LogicBase* construct(void) { return new ReflectorLogic; }
}


/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/

namespace {
  MsgProtoVer proto_ver;
};


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

ReflectorLogic::ReflectorLogic(void)
  : m_msg_type(0), m_udp_sock(0),
    m_logic_con_in(0), m_logic_con_out(0),
    m_reconnect_timer(60000, Timer::TYPE_ONESHOT, false),
    /*m_next_udp_tx_seq(0),*/ m_next_udp_rx_seq(0),
    m_heartbeat_timer(1000, Timer::TYPE_PERIODIC, false), m_dec(0),
    m_flush_timeout_timer(3000, Timer::TYPE_ONESHOT, false),
    m_udp_heartbeat_tx_cnt_reset(DEFAULT_UDP_HEARTBEAT_TX_CNT_RESET),
    m_udp_heartbeat_tx_cnt(0), m_udp_heartbeat_rx_cnt(0),
    m_tcp_heartbeat_tx_cnt(0), m_tcp_heartbeat_rx_cnt(0),
    m_con_state(STATE_DISCONNECTED), m_enc(0), m_default_tg(0),
    m_tg_select_timeout(DEFAULT_TG_SELECT_TIMEOUT),
    m_tg_select_inhibit_timeout(DEFAULT_TG_SELECT_TIMEOUT),
    m_tg_select_timer(1000, Async::Timer::TYPE_PERIODIC),
    m_tg_select_timeout_cnt(0), m_selected_tg(0), m_previous_tg(0),
    m_event_handler(0),
    m_report_tg_timer(500, Async::Timer::TYPE_ONESHOT, false),
    m_tg_local_activity(false), m_last_qsy(0), m_logic_con_in_valve(0),
    m_mute_first_tx_loc(true), m_mute_first_tx_rem(false),
    m_tmp_monitor_timer(1000, Async::Timer::TYPE_PERIODIC),
    m_tmp_monitor_timeout(DEFAULT_TMP_MONITOR_TIMEOUT), m_use_prio(true),
    m_qsy_pending_timer(-1), m_verbose(true)
{
  m_reconnect_timer.expired.connect(
      sigc::hide(mem_fun(*this, &ReflectorLogic::reconnect)));
  m_heartbeat_timer.expired.connect(
      mem_fun(*this, &ReflectorLogic::handleTimerTick));
  m_flush_timeout_timer.expired.connect(
      mem_fun(*this, &ReflectorLogic::flushTimeout));
  timerclear(&m_last_talker_timestamp);

  m_tg_select_timer.expired.connect(sigc::hide(
        sigc::mem_fun(*this, &ReflectorLogic::tgSelectTimerExpired)));
  m_report_tg_timer.expired.connect(sigc::hide(
        sigc::mem_fun(*this, &ReflectorLogic::processTgSelectionEvent)));
  m_tmp_monitor_timer.expired.connect(sigc::hide(
        sigc::mem_fun(*this, &ReflectorLogic::checkTmpMonitorTimeout)));
  m_qsy_pending_timer.expired.connect(sigc::hide(
        sigc::mem_fun(*this, &ReflectorLogic::qsyPendingTimeout)));

  m_con.connected.connect(
      sigc::mem_fun(*this, &ReflectorLogic::onConnected));
  m_con.disconnected.connect(
      sigc::mem_fun(*this, &ReflectorLogic::onDisconnected));
  m_con.frameReceived.connect(
      sigc::mem_fun(*this, &ReflectorLogic::onFrameReceived));
  m_con.verifyPeer.connect(
      sigc::mem_fun(*this, &ReflectorLogic::onVerifyPeer));
  m_con.sslConnectionReady.connect(
      sigc::mem_fun(*this, &ReflectorLogic::onSslConnectionReady));
  m_con.setMaxFrameSize(ReflectorMsg::MAX_PREAUTH_FRAME_SIZE);
} /* ReflectorLogic::ReflectorLogic */


bool ReflectorLogic::initialize(Async::Config& cfgobj, const std::string& logic_name)
{
    // Must create logic connection objects before calling LogicBase::initialize
  m_logic_con_in = new Async::AudioStreamStateDetector;
  m_logic_con_in->sigStreamStateChanged.connect(
      sigc::mem_fun(*this, &ReflectorLogic::onLogicConInStreamStateChanged));
  m_logic_con_out = new Async::AudioStreamStateDetector;
  m_logic_con_out->sigStreamStateChanged.connect(
      sigc::mem_fun(*this, &ReflectorLogic::onLogicConOutStreamStateChanged));

  if (!LogicBase::initialize(cfgobj, logic_name))
  {
    return false;
  }

  cfg().getValue(name(), "VERBOSE", m_verbose);

  std::vector<std::string> hosts;
  if (cfg().getValue(name(), "HOST", hosts))
  {
    std::cout << "*** WARNING: The " << name()
              << "/HOST configuration variable is deprecated. "
                 "Use HOSTS instead." << std::endl;
  }
  cfg().getValue(name(), "HOSTS", hosts);
  std::string srv_domain;
  cfg().getValue(name(), "DNS_DOMAIN", srv_domain);
  if (srv_domain.empty() && hosts.empty())
  {
    std::cerr << "*** ERROR: At least one of HOSTS or DNS_DOMAIN must be "
                 "specified in " << name() << std::endl;
     return false;
  }

  if (!srv_domain.empty())
  {
    m_con.setService("svxreflector", "tcp", srv_domain);
  }
  if (!hosts.empty())
  {
    uint16_t reflector_port = 5300;
    if (cfg().getValue(name(), "PORT", reflector_port))
    {
      std::cout << "*** WARNING: The " << name()
                << "/PORT configuration variable is deprecated. "
                   "Use HOST_PORT instead." << std::endl;
    }
    cfg().getValue(name(), "HOST_PORT", reflector_port);
    DnsResourceRecordSRV::Prio prio = 100;
    cfg().getValue(name(), "HOST_PRIO", prio);
    DnsResourceRecordSRV::Prio prio_inc = 1;
    cfg().getValue(name(), "HOST_PRIO_INC", prio_inc);
    DnsResourceRecordSRV::Weight weight = 100 / hosts.size();
    cfg().getValue(name(), "HOST_WEIGHT", weight);
    for (const auto& host_spec : hosts)
    {
      std::string host = host_spec;
      uint16_t port = reflector_port;
      auto colon = host.find(':');
      if (colon != std::string::npos)
      {
        host = host_spec.substr(0, colon);
        port = atoi(host_spec.substr(colon+1).c_str());
      }
      m_con.addStaticSRVRecord(0, prio, weight, port, host);
      prio += prio_inc;
    }
  }

  if (!cfg().getValue(name(), "CERT_PKI_DIR", m_pki_dir) || m_pki_dir.empty())
  {
    m_pki_dir = std::string(SVX_LOCAL_STATE_DIR) + "/pki";
  }
  if (!m_pki_dir.empty() && (access(m_pki_dir.c_str(), F_OK) != 0))
  {
    std::cout << name()
              << ": Create PKI directory \"" << m_pki_dir << "\""
              << std::endl;
    if (mkdir(m_pki_dir.c_str(), 0755) != 0)
    {
      std::cerr << "*** ERROR: Could not create PKI directory \""
                << m_pki_dir << "\" in logic \"" << name() << "\""
                << std::endl;
      return false;
    }
  }

  if (!cfg().getValue(name(), "CALLSIGN", m_callsign) || m_callsign.empty())
  {
    std::cerr << "*** ERROR: " << name()
              << "/CALLSIGN missing in configuration or is empty"
              << std::endl;
    return false;
  }

  if (!cfg().getValue(name(), "CERT_KEYFILE", m_keyfile))
  {
    m_keyfile = m_pki_dir + "/" + m_callsign + ".key";
  }
  if (access(m_keyfile.c_str(), F_OK) != 0)
  {
    std::cout << name()
              << ": Certificate key file not found. Generating key file \""
              << m_keyfile << "\"" << std::endl;
    Async::SslKeypair keypair;
    keypair.generate(2048);
    if (!keypair.writePrivateKeyFile(m_keyfile))
    {
      std::cerr << "*** ERROR: Failed to write private key file to \""
                << m_keyfile << "\" in logic \"" << name() << "\""
                << std::endl;
      return false;
    }
  }
  if (!m_ssl_pkey.readPrivateKeyFile(m_keyfile))
  {
    std::cerr << "*** ERROR: Failed to read private key file from \""
              << m_keyfile << "\" in logic \"" << name() << "\""
              << std::endl;
    return false;
  }

  m_ssl_csr.setVersion(Async::SslCertSigningReq::VERSION_1);
  m_ssl_csr.addSubjectName("CN", m_callsign);
  const std::vector<std::vector<std::string>> subject_names{
    {SN_givenName,              LN_givenName,               "GIVEN_NAME"},
    {SN_surname,                LN_surname,                 "SURNAME"},
    {SN_organizationalUnitName, LN_organizationalUnitName,  "ORG_UNIT"},
    {SN_organizationName,       LN_organizationName,        "ORG"},
    {SN_localityName,           LN_localityName,            "LOCALITY"},
    {SN_stateOrProvinceName,    LN_stateOrProvinceName,     "STATE"},
    {SN_countryName,            LN_countryName,             "COUNTRY"},
  };
  std::string value;
  const std::string prefix = "CERT_SUBJ_";
  for (const auto& snames : subject_names)
  {
    if (std::accumulate(snames.begin(), snames.end(), false,
          [&](bool found, const std::string& cfgsname)
          {
            return found || cfg().getValue(name(), prefix + cfgsname, value);
          }) &&
        !value.empty())
    {
      if (!m_ssl_csr.addSubjectName(snames[0], value))
      {
        std::cerr << "*** ERROR: Failed to set subject name '" << snames[0]
                  << "' in certificate signing request." << std::endl;
        return false;
      }
    }
  }
  Async::SslX509Extensions csr_exts;
  csr_exts.addBasicConstraints("critical, CA:FALSE");
  csr_exts.addKeyUsage(
      "critical, digitalSignature, keyEncipherment, keyAgreement");
  csr_exts.addExtKeyUsage("clientAuth");
  std::vector<std::string> cert_email;
  if (cfg().getValue(name(), "CERT_EMAIL", cert_email) && !cert_email.empty())
  {
    std::string csr_san;
    for (const auto& email : cert_email)
    {
      if (!csr_san.empty())
      {
        csr_san += ",";
      }
      csr_san += std::string("email:") + email;
    }
    csr_exts.addSubjectAltNames(csr_san);
  }
  m_ssl_csr.addExtensions(csr_exts);
  m_ssl_csr.setPublicKey(m_ssl_pkey);
  m_ssl_csr.sign(m_ssl_pkey);

  if (!cfg().getValue(name(), "CERT_CSRFILE", m_csrfile))
  {
    m_csrfile = m_pki_dir + "/" + m_callsign + ".csr";
  }
  Async::SslCertSigningReq req(nullptr);
  if (!req.readPemFile(m_csrfile) || !req.verify(m_ssl_pkey) ||
      (m_ssl_csr.digest() != req.digest()))
  {
    std::cout << name() << ": Saving certificate signing request file '"
              << m_csrfile << "'" << std::endl;
    //std::cout << "### New CSR" << std::endl;
    if (!m_ssl_csr.writePemFile(m_csrfile))
    {
      // FIXME: Read SSL error stack

      std::cerr << "*** ERROR: Failed to write certificate signing "
                   "request file to '"
                << m_csrfile << "' in logic '" << name() << "'"
                << std::endl;
      return false;
    }
  }
  //m_ssl_csr.print();

  if (!cfg().getValue(name(), "CERT_CRTFILE", m_crtfile))
  {
    m_crtfile = m_pki_dir + "/" + m_callsign + ".crt";
  }

  if (!loadClientCertificate())
  {
    std::cerr << "*** WARNING[" << name() << "]: Failed to load client "
                 "certificate. Ifnoring on-disk stored certificate file '"
              << m_crtfile << "'." << std::endl;
  }

  cfg().getValue(name(), "CERT_DOWNLOAD_CA_BUNDLE", m_download_ca_bundle);
  if (!cfg().getValue(name(), "CERT_CAFILE", m_cafile))
  {
    m_cafile = m_pki_dir + "/ca-bundle.crt";
  }
  if (!m_ssl_ctx.setCaCertificateFile(m_cafile))
  {
    if (m_download_ca_bundle)
    {
      std::cerr << "*** WARNING[" << name() << "]: Failed to read CA file '"
                << m_cafile << "'. Will try to retrieve it from the server."
                << std::endl;
    }
    else
    {
      std::cerr << "*** ERROR[" << name() << "]: Failed to read CA file '"
                << m_cafile << "' and CERT_DOWNLOAD_CA_BUNDLE is false."
                << std::endl;
      return false;
    }
  }

  string event_handler_str;
  if (!cfg().getValue(name(), "EVENT_HANDLER", event_handler_str) ||
      event_handler_str.empty())
  {
    std::cerr << "*** ERROR: Config variable " << name()
              << "/EVENT_HANDLER not set or empty" << std::endl;
    return false;
  }

  std::vector<std::string> monitor_tgs;
  cfg().getValue(name(), "MONITOR_TGS", monitor_tgs);
  for (std::vector<std::string>::iterator it=monitor_tgs.begin();
       it!=monitor_tgs.end(); ++it)
  {
    std::istringstream is(*it);
    MonitorTgEntry mte;
    is >> mte.tg;
    char modifier;
    while (is >> modifier)
    {
      if (modifier == '+')
      {
        mte.prio += 1;
      }
      else
      {
        cerr << "*** ERROR: Illegal format for config variable MONITOR_TGS "
             << "entry \"" << *it << "\"" << endl;
        return false;
      }
    }
    m_monitor_tgs.insert(mte);
  }

#if 0
  string audio_codec("GSM");
  if (AudioDecoder::isAvailable("OPUS") && AudioEncoder::isAvailable("OPUS"))
  {
    audio_codec = "OPUS";
  }
  else if (AudioDecoder::isAvailable("SPEEX") &&
           AudioEncoder::isAvailable("SPEEX"))
  {
    audio_codec = "SPEEX";
  }
  cfg().getValue(name(), "AUDIO_CODEC", audio_codec);
#endif

  AudioSource *prev_src = m_logic_con_in;

  cfg().getValue(name(), "MUTE_FIRST_TX_LOC", m_mute_first_tx_loc);
  cfg().getValue(name(), "MUTE_FIRST_TX_REM", m_mute_first_tx_rem);
  if (m_mute_first_tx_loc || m_mute_first_tx_rem)
  {
    m_logic_con_in_valve = new Async::AudioValve;
    m_logic_con_in_valve->setOpen(false);
    prev_src->registerSink(m_logic_con_in_valve);
    prev_src = m_logic_con_in_valve;
  }

  m_enc_endpoint = prev_src;
  prev_src = 0;

    // Create dummy audio codec used before setting the real encoder
  if (!setAudioCodec("DUMMY")) { return false; }
  prev_src = m_dec;

    // Create jitter buffer
  AudioFifo *fifo = new Async::AudioFifo(2*INTERNAL_SAMPLE_RATE);
  prev_src->registerSink(fifo, true);
  prev_src = fifo;
  unsigned jitter_buffer_delay = 0;
  cfg().getValue(name(), "JITTER_BUFFER_DELAY", jitter_buffer_delay);
  if (jitter_buffer_delay > 0)
  {
    fifo->setPrebufSamples(jitter_buffer_delay * INTERNAL_SAMPLE_RATE / 1000);
  }

  prev_src->registerSink(m_logic_con_out, true);
  prev_src = 0;

  cfg().getValue(name(), "DEFAULT_TG", m_default_tg);
  if (!cfg().getValue(name(), "TG_SELECT_TIMEOUT", 1U,
                      std::numeric_limits<unsigned>::max(),
                      m_tg_select_timeout, true))
  {
    std::cerr << "*** ERROR[" << name()
              << "]: Illegal value (" << m_tg_select_timeout
              << ") for TG_SELECT_TIMEOUT" << std::endl;
    return false;
  }

  m_tg_select_inhibit_timeout = m_tg_select_timeout;
  if (!cfg().getValue(name(), "TG_SELECT_INHIBIT_TIMEOUT", 0U,
                      std::numeric_limits<unsigned>::max(),
                      m_tg_select_inhibit_timeout, true))
  {
    std::cout << "*** ERROR[" << name()
              << "]: Illegal value (" << m_tg_select_inhibit_timeout
              << ") for TG_SELECT_INHIBIT_TIMEOUT" << std::endl;
    return false;
  }

  int qsy_pending_timeout = -1;
  if (cfg().getValue(name(), "QSY_PENDING_TIMEOUT", qsy_pending_timeout) &&
      (qsy_pending_timeout > 0))
  {
    m_qsy_pending_timer.setTimeout(1000 * qsy_pending_timeout);
  }

  m_event_handler = new EventHandler(event_handler_str, name());
  if (LinkManager::hasInstance())
  {
    m_event_handler->playFile.connect(
          sigc::mem_fun(*this, &ReflectorLogic::handlePlayFile));
    m_event_handler->playSilence.connect(
          sigc::mem_fun(*this, &ReflectorLogic::handlePlaySilence));
    m_event_handler->playTone.connect(
          sigc::mem_fun(*this, &ReflectorLogic::handlePlayTone));
    m_event_handler->playDtmf.connect(
          sigc::mem_fun(*this, &ReflectorLogic::handlePlayDtmf));
  }
  m_event_handler->setConfigValue.connect(
      sigc::mem_fun(cfg(), &Async::Config::setValue<std::string>));
  m_event_handler->setVariable("logic_name", name().c_str());

  m_event_handler->processEvent("namespace eval Logic {}");
  list<string> cfgvars = cfg().listSection(name());
  list<string>::const_iterator cfgit;
  for (cfgit=cfgvars.begin(); cfgit!=cfgvars.end(); ++cfgit)
  {
    string var = "Logic::CFG_" + *cfgit;
    string value;
    cfg().getValue(name(), *cfgit, value);
    m_event_handler->setVariable(var, value);
  }

  if (!m_event_handler->initialize())
  {
    return false;
  }

  cfg().getValue(name(), "TMP_MONITOR_TIMEOUT", m_tmp_monitor_timeout);

  std::string node_info_file;
  if (cfg().getValue(name(), "NODE_INFO_FILE", node_info_file))
  {
    std::ifstream node_info_is(node_info_file.c_str(), std::ios::in);
    if (node_info_is.good())
    {
      try
      {
        if (!(node_info_is >> m_node_info))
        {
          std::cerr << "*** ERROR: Failure while reading node information file "
                       "\"" << node_info_file << "\""
                    << std::endl;
          return false;
        }
      }
      catch (const Json::Exception& e)
      {
        std::cerr << "*** ERROR: Failure while reading node information "
                     "file \"" << node_info_file << "\": "
                  << e.what()
                  << std::endl;
        return false;
      }
    }
    else
    {
      std::cerr << "*** ERROR: Could not open node information file "
                   "\"" << node_info_file << "\""
                << std::endl;
      return false;
    }
  }
  m_node_info["sw"] = "SvxLink";
  m_node_info["swVer"] = SVXLINK_APP_VERSION;
  m_node_info["projVer"] = PROJECT_VERSION;

  cfg().getValue(name(), "UDP_HEARTBEAT_INTERVAL",
      m_udp_heartbeat_tx_cnt_reset);

  connect();

  return true;
} /* ReflectorLogic::initialize */


void ReflectorLogic::remoteCmdReceived(LogicBase* src_logic,
                                       const std::string& cmd)
{
  //cout << "### src_logic=" << src_logic->name() << "  cmd=" << cmd << endl;
  if (cmd == "*")
  {
    processEvent("report_tg_status");
  }
  //else if (cmd[0] == '0') // Help
  //{

  //}
  else if (cmd[0] == '1') // Select TG
  {
    const std::string subcmd(cmd.substr(1));
    if (!subcmd.empty()) // Select specified TG
    {
      istringstream is(subcmd);
      uint32_t tg;
      if (is >> tg)
      {
        selectTg(tg, "tg_command_activation", true);
        m_tg_local_activity = true;
        m_use_prio = false;
      }
      else
      {
        processEvent(std::string("command_failed ") + cmd);
      }
    }
    else // Select previous TG
    {
      selectTg(m_previous_tg, "tg_command_activation", true);
      m_tg_local_activity = true;
      m_use_prio = false;
    }
  }
  else if (cmd[0] == '2')   // QSY
  {
    if ((m_selected_tg != 0) && isLoggedIn())
    {
      const std::string subcmd(cmd.substr(1));
      if (subcmd.empty())
      {
        cout << name() << ": Requesting QSY to random TG" << endl;
        sendMsg(MsgRequestQsy());
      }
      else
      {
        std::istringstream is(subcmd);
        uint32_t tg = 0;
        if (is >> tg)
        {
          cout << name() << ": Requesting QSY to TG #" << tg << endl;
          sendMsg(MsgRequestQsy(tg));
        }
        else
        {
          processEvent("tg_qsy_failed");
        }
      }
    }
    else
    {
      processEvent("tg_qsy_failed");
    }
  }
  else if (cmd == "3")   // Follow last QSY
  {
    if ((m_last_qsy > 0) && (m_last_qsy != m_selected_tg))
    {
      selectTg(m_last_qsy, "tg_command_activation", true);
      m_tg_local_activity = true;
      m_use_prio = false;
    }
    else
    {
      processEvent(std::string("command_failed ") + cmd);
    }
  }
  else if (cmd[0] == '4')   // Temporarily monitor talk group
  {
    std::ostringstream os;
    const std::string subcmd(cmd.substr(1));
    if ((m_tmp_monitor_timeout > 0) && !subcmd.empty())
    {
      istringstream is(subcmd);
      uint32_t tg = 0;
      if (is >> tg)
      {
        const MonitorTgsSet::iterator it = m_monitor_tgs.find(tg);
        if (it != m_monitor_tgs.end())
        {
          if ((*it).timeout > 0)
          {
            std::cout << name() << ": Refresh temporary monitor for TG #"
                      << tg << std::endl;
              // NOTE: (*it).timeout is mutable
            (*it).timeout = m_tmp_monitor_timeout;
            os << "tmp_monitor_add " << tg;
          }
          else
          {
            std::cout << "*** WARNING: Not allowed to add a temporary montior "
                         "for TG #" << tg << " which is being permanently "
                         "monitored" << std::endl;
            os << "command_failed " << cmd;
          }
        }
        else
        {
          std::cout << name() << ": Add temporary monitor for TG #"
                    << tg << std::endl;
          MonitorTgEntry mte(tg);
          mte.timeout = m_tmp_monitor_timeout;
          m_monitor_tgs.insert(mte);
          sendMsg(MsgTgMonitor(std::set<uint32_t>(
                  m_monitor_tgs.begin(), m_monitor_tgs.end())));
          os << "tmp_monitor_add " << tg;
        }
      }
      else
      {
        std::cout << "*** WARNING: Failed to parse temporary TG monitor "
                     "command: " << cmd << std::endl;
        os << "command_failed " << cmd;
      }
    }
    else
    {
      std::cout << "*** WARNING: Ignoring temporary TG monitoring command ("
                << cmd << ") since that function is not enabled or there "
                   "were no TG specified" << std::endl;
      os << "command_failed " << cmd;
    }
    processEvent(os.str());
  }
  else
  {
    processEvent(std::string("unknown_command ") + cmd);
  }
} /* ReflectorLogic::remoteCmdReceived */


void ReflectorLogic::remoteReceivedTgUpdated(LogicBase *logic, uint32_t tg)
{
  //cout << "### ReflectorLogic::remoteReceivedTgUpdated: logic="
  //     << logic->name() << "  tg=" << tg
  //     << "  m_mute_first_tx_loc=" << m_mute_first_tx_loc << endl;
  if ((m_selected_tg == 0) && (tg > 0))
  {
    selectTg(tg, "tg_local_activation", !m_mute_first_tx_loc);
    m_tg_local_activity = !m_mute_first_tx_loc;
    m_use_prio = false;
  }
} /* ReflectorLogic::remoteReceivedTgUpdated */


void ReflectorLogic::remoteReceivedPublishStateEvent(
    LogicBase *logic, const std::string& event_name, const std::string& data)
{
  //cout << "### ReflectorLogic::remoteReceivedPublishStateEvent:"
  //     << " logic=" << logic->name()
  //     << " event_name=" << event_name
  //     << " data=" << data
  //     << endl;
  //sendMsg(MsgStateEvent(logic->name(), event_name, msg));

  if (event_name == "Voter:sql_state")
  {
    //MsgUdpSignalStrengthValues msg;
    MsgSignalStrengthValues msg;
    std::istringstream is(data);
    Json::Value rx_arr;
    is >> rx_arr;
    for (Json::Value::ArrayIndex i = 0; i != rx_arr.size(); i++)
    {
      Json::Value& rx_data = rx_arr[i];
      std::string name = rx_data.get("name", "").asString();
      std::string id_str = rx_data.get("id", "?").asString();
      if (id_str.size() != 1)
      {
        return;
      }
      char id = id_str[0];
      int siglev = rx_data.get("siglev", 0).asInt();
      siglev = std::min(std::max(siglev, 0), 100);
      bool is_enabled = rx_data.get("enabled", false).asBool();
      bool sql_open = rx_data.get("sql_open", false).asBool();
      bool is_active = rx_data.get("active", false).asBool();
      //MsgUdpSignalStrengthValues::Rx rx(id, siglev);
      MsgSignalStrengthValues::Rx rx(id, siglev);
      rx.setEnabled(is_enabled);
      rx.setSqlOpen(sql_open);
      rx.setActive(is_active);
      msg.pushBack(rx);
    }
    //sendUdpMsg(msg);
    sendMsg(msg);
  }
  else if (event_name == "Rx:sql_state")
  {
    //MsgUdpSignalStrengthValues msg;
    MsgSignalStrengthValues msg;
    std::istringstream is(data);
    Json::Value rx_data;
    is >> rx_data;
    std::string name = rx_data.get("name", "").asString();
    std::string id_str = rx_data.get("id", "?").asString();
    if (id_str.size() != 1)
    {
      return;
    }
    char id = id_str[0];
    int siglev = rx_data.get("siglev", 0).asInt();
    siglev = std::min(std::max(siglev, 0), 100);
    bool sql_open = rx_data.get("sql_open", false).asBool();
    //MsgUdpSignalStrengthValues::Rx rx(id, siglev);
    MsgSignalStrengthValues::Rx rx(id, siglev);
    rx.setEnabled(true);
    rx.setSqlOpen(sql_open);
    rx.setActive(sql_open);
    msg.pushBack(rx);
    //sendUdpMsg(msg);
    sendMsg(msg);
  }
  else if (event_name == "Tx:state")
  {
    MsgTxStatus msg;
    std::istringstream is(data);
    Json::Value tx_data;
    is >> tx_data;
    std::string name = tx_data.get("name", "").asString();
    std::string id_str = tx_data.get("id", "?").asString();
    if (id_str.size() != 1)
    {
      return;
    }
    char id = id_str[0];
    if (id != '\0')
    {
      bool transmit = tx_data.get("transmit", false).asBool();
      MsgTxStatus::Tx tx(id);
      tx.setTransmit(transmit);
      msg.pushBack(tx);
      sendMsg(msg);
    }
  }
  else if (event_name == "MultiTx:state")
  {
    MsgTxStatus msg;
    std::istringstream is(data);
    Json::Value tx_arr;
    is >> tx_arr;
    for (Json::Value::ArrayIndex i = 0; i != tx_arr.size(); i++)
    {
      Json::Value& tx_data = tx_arr[i];
      std::string name = tx_data.get("name", "").asString();
      std::string id_str = tx_data.get("id", "").asString();
      if (id_str.size() != 1)
      {
        return;
      }
      char id = id_str[0];
      if (id != '\0')
      {
        bool transmit = tx_data.get("transmit", false).asBool();
        MsgTxStatus::Tx tx(id);
        tx.setTransmit(transmit);
        msg.pushBack(tx);
      }
    }
    sendMsg(msg);
  }
} /* ReflectorLogic::remoteReceivedPublishStateEvent */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

ReflectorLogic::~ReflectorLogic(void)
{
  disconnect();
  delete m_event_handler;
  m_event_handler = 0;
  delete m_udp_sock;
  m_udp_sock = 0;
  delete m_logic_con_in;
  m_logic_con_in = 0;
  delete m_enc;
  m_enc = 0;
  delete m_dec;
  m_dec = 0;
  delete m_logic_con_in_valve;
  m_logic_con_in_valve = 0;
} /* ReflectorLogic::~ReflectorLogic */




/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void ReflectorLogic::onConnected(void)
{
  std::cout << name() << ": Connection established to "
            << m_con.remoteHost() << ":" << m_con.remotePort()
            << " (" << (m_con.isPrimary() ? "primary" : "secondary") << ")"
            << std::endl;
  sendMsg(proto_ver);
  m_udp_heartbeat_tx_cnt = m_udp_heartbeat_tx_cnt_reset;
  m_udp_heartbeat_rx_cnt = UDP_HEARTBEAT_RX_CNT_RESET;
  m_tcp_heartbeat_tx_cnt = TCP_HEARTBEAT_TX_CNT_RESET;
  m_tcp_heartbeat_rx_cnt = TCP_HEARTBEAT_RX_CNT_RESET;
  m_heartbeat_timer.setEnable(true);
  //m_next_udp_tx_seq = 0;
  m_next_udp_rx_seq = 0;
  timerclear(&m_last_talker_timestamp);
  //m_con_state = STATE_EXPECT_AUTH_CHALLENGE;
  m_con.setMaxFrameSize(ReflectorMsg::MAX_PRE_SSL_SETUP_SIZE);
  m_con_state = STATE_EXPECT_CA_INFO;
  //m_con.setMaxFrameSize(ReflectorMsg::MAX_PREAUTH_FRAME_SIZE);
  processEvent("reflector_connection_status_update 1");
} /* ReflectorLogic::onConnected */


void ReflectorLogic::onDisconnected(TcpConnection*,
                                    TcpConnection::DisconnectReason reason)
{
  cout << name() << ": Disconnected from " << m_con.remoteHost() << ":"
       << m_con.remotePort() << ": "
       << TcpConnection::disconnectReasonStr(reason) << endl;
  //m_reconnect_timer.setTimeout(1000 + std::rand() % 5000);
  m_reconnect_timer.setEnable(reason == TcpConnection::DR_ORDERED_DISCONNECT);
  delete m_udp_sock;
  m_udp_sock = 0;
  //m_next_udp_tx_seq = 0;
  m_next_udp_rx_seq = 0;
  m_heartbeat_timer.setEnable(false);
  if (m_flush_timeout_timer.isEnabled())
  {
    m_flush_timeout_timer.setEnable(false);
    m_enc->allEncodedSamplesFlushed();
  }
  if (timerisset(&m_last_talker_timestamp))
  {
    m_dec->flushEncodedSamples();
    timerclear(&m_last_talker_timestamp);
  }
  m_con_state = STATE_DISCONNECTED;
  processEvent("reflector_connection_status_update 0");
} /* ReflectorLogic::onDisconnected */


bool ReflectorLogic::onVerifyPeer(TcpConnection *con, bool preverify_ok,
                                  X509_STORE_CTX *x509_store_ctx)
{
  //std::cout << "### ReflectorLogic::onVerifyPeer: preverify_ok="
  //          << (preverify_ok ? "yes" : "no") << std::endl;

  Async::SslX509 cert(*x509_store_ctx);
  preverify_ok = preverify_ok && !cert.isNull();
  preverify_ok = preverify_ok && !cert.commonName().empty();
  if (!preverify_ok)
  {
    std::cout << "*** ERROR[" << name()
              << "]: Certificate verification failed for reflector server"
              << std::endl;
    std::cout << "------------- Peer Certificate --------------" << std::endl;
    cert.print();
    std::cout << "---------------------------------------------" << std::endl;
  }

  return preverify_ok;
} /* ReflectorLogic::onVerifyPeer */


void ReflectorLogic::onSslConnectionReady(TcpConnection*)
{
  std::cout << name() << ": Encrypted connection established" << std::endl;

  if (m_con_state != STATE_EXPECT_SSL_CON_READY)
  {
    std::cerr << "*** ERROR["
              << name() << "]: Unexpected SSL connection readiness"
              << std::endl;
    disconnect();
    return;
  }

  if (m_con.sslVerifyResult() != X509_V_OK)
  {
    std::cerr << "*** ERROR["
              << name() << "]: SSL Certificate verification failed"
              << std::endl;
    disconnect();
    return;
  }

  auto peer_cert = m_con.sslPeerCertificate();
  //std::cout << "###   Common Name=" << peer_cert.commonName() << std::endl;

  bool cert_match_host = false;
  std::string remote_name = m_con.remoteHostName();
  if (!remote_name.empty())
  {
    if (remote_name[remote_name.size()-1] == '.')
    {
      remote_name.erase(remote_name.size()-1);
    }
    //std::cout << "### Remote hostname=" << remote_name << std::endl;
    cert_match_host |= peer_cert.matchHost(remote_name);
  }
  cert_match_host |= peer_cert.matchIp(m_con.remoteHost());
  if (!cert_match_host)
  {
    std::cerr << "*** EROR[" << name()
              << "]: The server certificate does not match the remote "
                 "hostname ("<< remote_name << ") nor the IP address ("
              << m_con.remoteHost() << ")"
              << std::endl;
    disconnect();
    return;
  }

  m_con.setMaxFrameSize(ReflectorMsg::MAX_POST_SSL_SETUP_SIZE);

  m_con_state = STATE_EXPECT_AUTH_ANSWER;
} /* ReflectorLogic::onSslConnectionReady */


void ReflectorLogic::onFrameReceived(FramedTcpConnection*,
                                     std::vector<uint8_t>& data)
{
  //std::cout << "### ReflectorLogic::onFrameReceived: data.size()="
  //          << data.size() << std::endl;
  char *buf = reinterpret_cast<char*>(&data.front());
  int len = data.size();

  stringstream ss;
  ss.write(buf, len);

  ReflectorMsg header;
  if (!header.unpack(ss))
  {
    std::cerr << "*** ERROR[" << name()
              << "]: Unpacking failed for TCP message header" << std::endl;
    disconnect();
    return;
  }

  if ((header.type() > 100) && !isTcpLoggedIn())
  {
    cerr << "*** ERROR[" << name() << "]: Unexpected protocol message received"
         << endl;
    disconnect();
    return;
  }

  m_tcp_heartbeat_rx_cnt = TCP_HEARTBEAT_RX_CNT_RESET;

  switch (header.type())
  {
    case MsgHeartbeat::TYPE:
      break;
    case MsgError::TYPE:
      handleMsgError(ss);
      break;
    case MsgProtoVerDowngrade::TYPE:
      handleMsgProtoVerDowngrade(ss);
      break;
    case MsgAuthChallenge::TYPE:
      handleMsgAuthChallenge(ss);
      break;
    case MsgAuthOk::TYPE:
      handleMsgAuthOk();
      break;
    case MsgCAInfo::TYPE:
      handleMsgCAInfo(ss);
      break;
    case MsgStartEncryption::TYPE:
      handleMsgStartEncryption();
      break;
    case MsgCABundle::TYPE:
      handleMsgCABundle(ss);
      break;
    case MsgClientCsrRequest::TYPE:
      handleMsgClientCsrRequest();
      break;
    case MsgClientCert::TYPE:
      handleMsgClientCert(ss);
      break;
    case MsgServerInfo::TYPE:
      handleMsgServerInfo(ss);
      break;
    case MsgNodeList::TYPE:
      handleMsgNodeList(ss);
      break;
    case MsgNodeJoined::TYPE:
      handleMsgNodeJoined(ss);
      break;
    case MsgNodeLeft::TYPE:
      handleMsgNodeLeft(ss);
      break;
    case MsgTalkerStart::TYPE:
      handleMsgTalkerStart(ss);
      break;
    case MsgTalkerStop::TYPE:
      handleMsgTalkerStop(ss);
      break;
    case MsgRequestQsy::TYPE:
      handleMsgRequestQsy(ss);
      break;
    case MsgStartUdpEncryption::TYPE:
      handlMsgStartUdpEncryption(ss);
      break;
    default:
      // Better just ignoring unknown messages for easier addition of protocol
      // messages while being backwards compatible

      //cerr << "*** WARNING[" << name()
      //     << "]: Unknown protocol message received: msg_type="
      //     << header.type() << endl;
      break;
  }
} /* ReflectorLogic::onFrameReceived */


void ReflectorLogic::handleMsgError(std::istream& is)
{
  MsgError msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgAuthError" << endl;
    disconnect();
    return;
  }
  cout << name() << ": Error message received from server: " << msg.message()
       << endl;
  disconnect();
} /* ReflectorLogic::handleMsgError */


void ReflectorLogic::handleMsgProtoVerDowngrade(std::istream& is)
{
  MsgProtoVerDowngrade msg;
  if (!msg.unpack(is))
  {
    std::cerr << "*** ERROR[" << name() 
              << "]: Could not unpack MsgProtoVerDowngrade" << std::endl;
    disconnect();
    return;
  }
#if 0
  if (msg.majorVer() == 2)
  {
    std::cout << name()
	      << ": The server is requesting protocol downgrade to v"
	      << msg.majorVer() << "." << msg.minorVer() << ". Complying."
	      << std::endl;
    sendMsg(MsgProtoVer(msg.majorVer(), msg.minorVer()));
    m_con_state = STATE_EXPECT_AUTH_CHALLENGE;
  }
  else
#endif
  {
    std::cout << name()
         << ": Server too old and we cannot downgrade to protocol version "
         << msg.majorVer() << "." << msg.minorVer() << " from "
         << proto_ver.majorVer() << "." << proto_ver.minorVer()
         << std::endl;
    disconnect();
  }
} /* ReflectorLogic::handleMsgProtoVerDowngrade */


void ReflectorLogic::handleMsgAuthChallenge(std::istream& is)
{
  if ((m_con_state != STATE_EXPECT_AUTH_ANSWER) /* &&
      (m_con_state != STATE_EXPECT_CERT) &&
      (m_con_state != STATE_EXPECT_AUTH_RESPONSE) */)
  {
    cerr << "*** ERROR[" << name() << "]: Unexpected MsgAuthChallenge\n";
    disconnect();
    return;
  }

  MsgAuthChallenge msg;
  if (!msg.unpack(is))
  {
    std::cerr << "*** ERROR[" << name()
              << "]: Could not unpack MsgAuthChallenge" << std::endl;
    disconnect();
    return;
  }
  const uint8_t *challenge = msg.challenge();
  if (challenge == 0)
  {
    cerr << "*** ERROR[" << name() << "]: Illegal challenge received\n";
    disconnect();
    return;
  }

  std::string auth_key;
  cfg().getValue(name(), "AUTH_KEY", auth_key);
  sendMsg(MsgAuthResponse(m_callsign, auth_key, challenge));
  //m_con_state = STATE_EXPECT_AUTH_ANSWER;
} /* ReflectorLogic::handleMsgAuthChallenge */


void ReflectorLogic::handleMsgAuthOk(void)
{
  if (m_con_state != STATE_EXPECT_AUTH_ANSWER)
  {
    cerr << "*** ERROR[" << name() << "]: Unexpected MsgAuthOk\n";
    disconnect();
    return;
  }
  std::cout << name() << ": Authentication OK" << std::endl;
  m_con_state = STATE_EXPECT_SERVER_INFO;
  m_con.setMaxFrameSize(ReflectorMsg::MAX_POSTAUTH_FRAME_SIZE);

  auto cert = m_con.sslCertificate();
  if (!cert.isNull())
  {
    struct stat csrst, crtst;
    if ((stat(m_csrfile.c_str(), &csrst) == 0) &&
        (stat(m_crtfile.c_str(), &crtst) == 0) &&
        (csrst.st_mtim.tv_sec > crtst.st_mtim.tv_sec))
    {
      //std::cout << "### CSR mtime=" << csrst.st_mtim.tv_sec
      //          << "  CRT mtime=" << crtst.st_mtim.tv_sec
      //          << std::endl;
      std::cout << name()
                << ": The CSR is newer than the certificate. Sending "
                   "certificate signing request to server." << std::endl;
      sendMsg(MsgClientCsr(m_ssl_csr.pem()));
    }
  }
} /* ReflectorLogic::handleMsgAuthOk */


void ReflectorLogic::handleMsgCAInfo(std::istream& is)
{
  if (m_con_state != STATE_EXPECT_CA_INFO)
  {
    std::cerr << "*** ERROR[" << name()
              << "]: Unexpected MsgCAInfo" << std::endl;
    disconnect();
    return;
  }

  MsgCAInfo msg;
  if (!msg.unpack(is))
  {
    std::cerr << "*** ERROR[" << name() << "]: Could not unpack MsgCAInfo"
              << std::endl;
    disconnect();
    return;
  }

  //std::cout << "### ca_size=" << msg.pemSize()
  //          //<< "  ca_url='" << msg.url() << "'"
  //          << std::endl;
  //std::cout << "### Message digest size: " << msg.md().size() << std::endl;
  //hexdump(msg.md());

  bool request_ca_bundle = false;
  if (m_download_ca_bundle)
  {
    std::ifstream ca_ifs(m_cafile);
    request_ca_bundle = !ca_ifs.good();
    if (ca_ifs.good())
    {
      std::string ca_pem(std::istreambuf_iterator<char>{ca_ifs}, {});
      auto ca_md = Async::Digest().md("sha256", ca_pem);
      request_ca_bundle = (ca_md != msg.md());
      if (request_ca_bundle)
      {
        //std::cout << "### Local CA PEM\n" << ca_pem << std::endl;
        //std::cout << "SHA256 Digest\n";
        //hexdump(ca_md);

          // FIXME: Don't overwrite CA bundle if we have one already. To do
          //        that we need to implement verification of the new bundle.
        std::cout << "*** WARNING[" << name()
                  << "]: You need to update your CA bundle to the latest "
                     "version. Contact the reflector sysop."  << std::endl;
        request_ca_bundle = false;
      }
    }
    ca_ifs.close();
  }
  if (request_ca_bundle)
  {
    //std::cout << "### Requesting CA Bundle" << std::endl;
    sendMsg(MsgCABundleRequest());
    m_con_state = STATE_EXPECT_CA_BUNDLE;
  }
  else
  {
    //std::cout << "### Requesting encrypted communications channel"
    //          << std::endl;
    m_con.setMaxFrameSize(ReflectorMsg::MAX_PRE_SSL_SETUP_SIZE);
    sendMsg(MsgStartEncryptionRequest());
    m_con_state = STATE_EXPECT_START_ENCRYPTION;
  }
} /* ReflectorLogic::handleMsgCAInfo */


void ReflectorLogic::handleMsgCABundle(std::istream& is)
{
  //std::cout << "### ReflectorLogic::handleMsgCABundle" << std::endl;

  if (m_con_state != STATE_EXPECT_CA_BUNDLE)
  {
    std::cerr << "*** ERROR[" << name()
         << "]: Unexpected MsgCABundle" << std::endl;
    disconnect();
    return;
  }
  MsgCABundle msg;
  if (!msg.unpack(is))
  {
    std::cerr << "*** ERROR[" << name() << "]: Could not unpack MsgCABundle"
              << std::endl;
    disconnect();
    return;
  }

  //std::cout << "### CA:\n" << msg.caPem() << std::endl;
  //std::cout << "### Signature:\n";
  //hexdump(msg.signature());
  Async::SslX509 signing_cert;
  signing_cert.readPem(msg.certPem());
  //std::cout << "### Signing cert chain:\n" << std::string(48, '-') << "\n";
  //signing_cert.print();
  //std::cout << std::string(48, '-') << "\n";

  if (msg.caPem().empty())
  {
    std::cerr << "*** ERROR[" << name() << "]: Received empty CA bundle"
              << std::endl;
    disconnect();
    return;
  }

  Async::Digest dgst;
  auto signing_cert_pubkey = signing_cert.publicKey();
  //std::cout << "### Signing public key:\n"
  //          << signing_cert_pubkey.publicKeyPem() << std::endl;
  bool signature_ok =
    dgst.signVerifyInit(MsgCABundle::MD_ALG, signing_cert_pubkey) &&
    dgst.signVerifyUpdate(msg.caPem()) &&
    dgst.signVerifyFinal(msg.signature());
  //std::cout << "### Signature check: " << (signature_ok ? "OK" : "FAIL")
  //          << std::endl;
  if (!signature_ok)
  {
    // FIXME: Add more info to the warning printout
    std::cout << "*** WARNING[" << name()
              << "]: Received CA bundle with invalid signature" << std::endl;
    disconnect();
    return;
  }

  // FIXME: Verify signing certificate against old CA, then write new CA
  //        bundle file if it's ok

  if (!msg.caPem().empty())
  {
    std::cout << name() << ": Writing received CA bundle to '" << m_cafile
              << "'" << std::endl;
    std::ofstream ofs(m_cafile);
    if (ofs.good())
    {
      ofs << msg.caPem();
      ofs.close();
    }
    else
    {
      std::cerr << "*** ERROR[" << name() << "]: Could not write CA file '"
                << m_cafile << "'" << std::endl;
    }

    if (!m_ssl_ctx.setCaCertificateFile(m_cafile))
    {
      std::cerr << "*** ERROR[" << name() << "]: Failed to read CA file '"
                << m_cafile << "'" << std::endl;
      disconnect();
      return;
    }
  }

  sendMsg(MsgStartEncryptionRequest());
  m_con_state = STATE_EXPECT_START_ENCRYPTION;
} /* ReflectorLogic::handleMsgCABundle */


void ReflectorLogic::handleMsgStartEncryption(void)
{
  //std::cout << "### ReflectorLogic::handleMsgStartEncryption" << std::endl;

  if (m_con_state != STATE_EXPECT_START_ENCRYPTION)
  {
    std::cerr << "*** ERROR[" << name()
         << "]: Unexpected MsgStartEncryption" << std::endl;
    disconnect();
    return;
  }

  std::cout << name() << ": Setting up encrypted communications channel"
            << std::endl;

  m_con.enableSsl(true);
  m_con_state = STATE_EXPECT_SSL_CON_READY;
} /* ReflectorLogic::handleMsgStartEncryption */


void ReflectorLogic::handleMsgClientCsrRequest(void)
{
  if (m_con_state != STATE_EXPECT_AUTH_ANSWER)
  {
    std::cerr << "*** ERROR[" << name() << "]: Unexpected MsgClientCsrRequest"
              << std::endl;
    disconnect();
    return;
  }

  //Async::SslCertSigningReq req;
  //if (!req.readPemFile(m_csrfile) || req.isNull())
  //{
  //  std::cerr << "*** ERROR[" << name() << "]: Missing or invalid CSR file "
  //               "'" << m_csrfile << "'" << std::endl;
  //  disconnect();
  //  return;
  //}

  std::cout << name() << ": Sending requested Certificate Signing Request "
                         "to server" << std::endl;

  sendMsg(MsgClientCsr(m_ssl_csr.pem()));
  //m_con_state = STATE_EXPECT_CERT;
} /* ReflectorLogic::handleMsgClientCsrRequest */


void ReflectorLogic::handleMsgClientCert(std::istream& is)
{
  if (m_con_state < STATE_EXPECT_AUTH_ANSWER)
  {
    std::cerr << "*** ERROR[" << name() << "]: Unexpected MsgClientCert"
              << std::endl;
    disconnect();
    return;
  }
  MsgClientCert msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgClientCert\n";
    disconnect();
    return;
  }

  if (msg.certPem().empty())
  {
    std::cout << name() << ": Received an empty certificate. "
              //<< "Sending Certificate Signing Request to server."
              << std::endl;
    //sendMsg(MsgClientCsr(m_ssl_csr.pem()));
    disconnect();
    return;
  }

  std::cout << name() << ": Received certificate from server"
            << std::endl;
  Async::SslX509 cert;
  if (!cert.readPem(msg.certPem()) || cert.isNull())
  {
    std::cerr << "*** ERROR[" << name()
              << "]: Failed to parse certificate PEM data from server"
              << std::endl;
    disconnect();
    return;
  }
  std::cout << "---------- New Client Certificate -----------" << std::endl;
  cert.print();
  std::cout << "---------------------------------------------" << std::endl;

  if (cert.publicKey() != m_ssl_csr.publicKey())
  {
    std::cerr << "*** ERROR[" << name() << "]: The client certificate "
                 "received from the server does not match our current "
                 "private key. "
                 //"Sending Certificate Signing Request."
              << std::endl;
    //sendMsg(MsgClientCsr(m_ssl_csr.pem()));
    disconnect();
    return;
  }

  std::ofstream ofs(m_crtfile);
  if (!ofs.good() || !(ofs << msg.certPem()))
  {
    std::cerr << "*** ERROR[" << name()
              << "]: Failed to write certificate file to \""
              << m_crtfile << "\"" << std::endl;
    disconnect();
    return;
  }
  ofs.close();

  //if (!cert.writePemFile(m_crtfile))
  //{
  //  std::cerr << "*** ERROR[" << name()
  //            << "]: Failed to write certificate file to \""
  //            << m_crtfile << "\"" << std::endl;
  //  disconnect();
  //  return;
  //}

  if (!loadClientCertificate())
  {
    std::cout << name() << ": Failed to load client certificate. "
              //<< "Sending certificate signing request to server"
              << std::endl;
    //sendMsg(MsgClientCsr(m_ssl_csr.pem()));
    disconnect();
    return;
  }

  reconnect();
} /* ReflectorLogic::handleMsgClientCert */


void ReflectorLogic::handleMsgServerInfo(std::istream& is)
{
  if (m_con_state != STATE_EXPECT_SERVER_INFO)
  {
    cerr << "*** ERROR[" << name() << "]: Unexpected MsgServerInfo\n";
    disconnect();
    return;
  }
  MsgServerInfo msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgServerInfo\n";
    disconnect();
    return;
  }
  m_client_id = msg.clientId();

  //cout << "### MsgServerInfo: clientId=" << msg.clientId()
  //     << " codecs=";
  //std::copy(msg.codecs().begin(), msg.codecs().end(),
  //     std::ostream_iterator<std::string>(cout, " "));
  //cout << " nodes=";
  //std::copy(msg.nodes().begin(), msg.nodes().end(),
  //     std::ostream_iterator<std::string>(cout, " "));
  //cout << endl;

  cout << name() << ": Connected nodes: ";
  const vector<string>& nodes = msg.nodes();
  if (!nodes.empty())
  {
    vector<string>::const_iterator it = nodes.begin();
    cout << *it++;
    for (; it != nodes.end(); ++it)
    {
      cout << ", " << *it;
    }
  }
  cout << endl;

  string selected_codec;
  for (vector<string>::const_iterator it = msg.codecs().begin();
       it != msg.codecs().end();
       ++it)
  {
    if (codecIsAvailable(*it))
    {
      selected_codec = *it;
      setAudioCodec(selected_codec);
      break;
    }
  }
  cout << name() << ": ";
  if (!selected_codec.empty())
  {
    std::cout << "Using audio codec \"" << selected_codec << "\"" << std::endl;
  }
  else
  {
    std::cout << "No supported codec :-(" << std::endl;
    disconnect();
    return;
  }

  /*
  const Async::EncryptedUdpSocket::Cipher* cipher = nullptr;
  for (const auto& cipher_name : msg.udpCiphers())
  {
    cipher = EncryptedUdpSocket::fetchCipher(cipher_name);
    if (cipher != nullptr)
    {
      break;
    }
  }
  */
  std::cout << name() << ": ";
  const auto cipher = EncryptedUdpSocket::fetchCipher(UdpCipher::NAME);
  if (cipher != nullptr)
  {
    std::cout << "Using UDP cipher " << EncryptedUdpSocket::cipherName(cipher)
              << std::endl;
  }
  else
  {
    std::cout << "Unsupported UDP cipher " << UdpCipher::NAME
              << " :-(" << std::endl;
    disconnect();
    return;
  }

  delete m_udp_sock;
  m_udp_cipher_iv_cntr = 1;
  m_udp_sock = new Async::EncryptedUdpSocket;
  m_udp_cipher_iv_rand.resize(UdpCipher::IVRANDLEN);
  const char* err = "unknown reason";
  if ((err="memory allocation failure",     (m_udp_sock == nullptr)) ||
      (err="initialization failure",        !m_udp_sock->initOk()) ||
      (err="unsupported cipher",            !m_udp_sock->setCipher(cipher)) ||
      (err="cipher IV rand generation failure",
       !Async::EncryptedUdpSocket::randomBytes(m_udp_cipher_iv_rand)) ||
      (err="cipher key generation failure", !m_udp_sock->setCipherKey()))
  {
    std::cerr << "*** ERROR[" << name() << "]: Could not create UDP socket "
                 "due to " << err << std::endl;
    delete m_udp_sock;
    m_udp_sock = nullptr;
    disconnect();
    return;
  }
  m_udp_sock->setCipherAADLength(UdpCipher::AADLEN);
  m_udp_sock->setTagLength(UdpCipher::TAGLEN);
  m_udp_sock->cipherDataReceived.connect(
      mem_fun(*this, &ReflectorLogic::udpCipherDataReceived));
  m_udp_sock->dataReceived.connect(
      mem_fun(*this, &ReflectorLogic::udpDatagramReceived));

  m_con_state = STATE_EXPECT_START_UDP_ENCRYPTION;

  std::ostringstream node_info_os;
  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = ""; //The JSON document is written on a single line
  Json::StreamWriter* writer = builder.newStreamWriter();
  writer->write(m_node_info, &node_info_os);
  delete writer;
  MsgNodeInfo node_info_msg(m_udp_cipher_iv_rand, m_udp_sock->cipherKey(),
                            node_info_os.str());
  sendMsg(node_info_msg);

#if 0
    // Set up RX and TX sites node information
  MsgNodeInfo::RxSite rx_site;
  MsgNodeInfo::TxSite tx_site;
  rx_site.setRxName("Rx1");
  tx_site.setTxName("Tx1");
  rx_site.setQthName(cfg().getValue("LocationInfo", "QTH_NAME"));
  tx_site.setQthName(cfg().getValue("LocationInfo", "QTH_NAME"));
  int32_t antenna_height = 0;
  if (cfg().getValue("LocationInfo", "ANTENNA_HEIGHT", antenna_height))
  {
    rx_site.setAntennaHeight(antenna_height);
    tx_site.setAntennaHeight(antenna_height);
  }
  float antenna_dir = -1.0f;
  cfg().getValue("LocationInfo", "ANTENNA_DIR", antenna_dir);
  rx_site.setAntennaDirection(antenna_dir);
  tx_site.setAntennaDirection(antenna_dir);
  double rf_frequency = 0;
  cfg().getValue("LocationInfo", "FREQUENCY", rf_frequency);
  if (rf_frequency < 0.0f)
  {
    rf_frequency = 0.0f;
  }
  rx_site.setRfFrequency(static_cast<uint64_t>(1000000.0f * rf_frequency));
  tx_site.setRfFrequency(static_cast<uint64_t>(1000000.0f * rf_frequency));
  vector<float> ctcss_frequencies;
  //ctcss_frequencies.push_back(136.5);
  //rx_site.setCtcssFrequencies(ctcss_frequencies);
  //tx_site.setCtcssFrequencies(ctcss_frequencies);
  float tx_power = 0.0f;
  cfg().getValue("LocationInfo", "TX_POWER", tx_power);
  tx_site.setTxPower(tx_power);
  MsgNodeInfo::TxSites tx_sites;
  tx_sites.push_back(tx_site);
  MsgNodeInfo::RxSites rx_sites;
  rx_sites.push_back(rx_site);

    // Send node information to the server
  MsgNodeInfo node_info; node_info
    .setSwInfo("SvxLink v" SVXLINK_VERSION)
    .setTxSites(tx_sites)
    .setRxSites(rx_sites);
  sendMsg(node_info);
#endif

  //if (m_selected_tg > 0)
  //{
  //  cout << name() << ": Selecting TG #" << m_selected_tg << endl;
  //  sendMsg(MsgSelectTG(m_selected_tg));
  //}

  //if (!m_monitor_tgs.empty())
  //{
  //  sendMsg(MsgTgMonitor(
  //        std::set<uint32_t>(m_monitor_tgs.begin(), m_monitor_tgs.end())));
  //}

  //sendUdpMsg(MsgUdpHeartbeat());
    // Send an empty datagram to open upp any firewalls
  //m_udp_sock->UdpSocket::write(
  //    m_con.remoteHost(), m_con.remotePort(), nullptr, 0);
  //sendUdpRegisterMsg();

} /* ReflectorLogic::handleMsgServerInfo */


void ReflectorLogic::handleMsgNodeList(std::istream& is)
{
  MsgNodeList msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgNodeList\n";
    disconnect();
    return;
  }
  cout << name() << ": Connected nodes: ";
  const vector<string>& nodes = msg.nodes();
  if (!nodes.empty())
  {
    vector<string>::const_iterator it = nodes.begin();
    cout << *it++;
    for (; it != nodes.end(); ++it)
    {
      cout << ", " << *it;
    }
  }
  cout << endl;
} /* ReflectorLogic::handleMsgNodeList */


void ReflectorLogic::handleMsgNodeJoined(std::istream& is)
{
  MsgNodeJoined msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgNodeJoined\n";
    disconnect();
    return;
  }
  if (m_verbose)
  {
    std::cout << name() << ": Node joined: " << msg.callsign() << std::endl;
  }
} /* ReflectorLogic::handleMsgNodeJoined */


void ReflectorLogic::handleMsgNodeLeft(std::istream& is)
{
  MsgNodeLeft msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgNodeLeft\n";
    disconnect();
    return;
  }
  if (m_verbose)
  {
    std::cout << name() << ": Node left: " << msg.callsign() << std::endl;
  }
} /* ReflectorLogic::handleMsgNodeLeft */


void ReflectorLogic::handleMsgTalkerStart(std::istream& is)
{
  MsgTalkerStart msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgTalkerStart\n";
    disconnect();
    return;
  }
  cout << name() << ": Talker start on TG #" << msg.tg() << ": "
       << msg.callsign() << endl;

    // Select the incoming TG if idle
  if (m_tg_select_timeout_cnt == 0)
  {
    selectTg(msg.tg(), "tg_remote_activation", !m_mute_first_tx_rem);
  }
  else if (m_use_prio)
  {
    uint32_t selected_tg_prio = 0;
    MonitorTgsSet::const_iterator selected_tg_it =
      m_monitor_tgs.find(MonitorTgEntry(m_selected_tg));
    if (selected_tg_it != m_monitor_tgs.end())
    {
      selected_tg_prio = selected_tg_it->prio;
    }
    MonitorTgsSet::const_iterator talker_tg_it =
      m_monitor_tgs.find(MonitorTgEntry(msg.tg()));
    if ((talker_tg_it != m_monitor_tgs.end()) &&
        (talker_tg_it->prio > selected_tg_prio))
    {
      std::cout << name() << ": Activity on prioritized TG #"
                << msg.tg() << ". Switching!" << std::endl;
      selectTg(msg.tg(), "tg_remote_prio_activation", !m_mute_first_tx_rem);
    }
  }

  std::ostringstream ss;
  ss << "talker_start " << msg.tg() << " " << msg.callsign();
  processEvent(ss.str());
} /* ReflectorLogic::handleMsgTalkerStart */


void ReflectorLogic::handleMsgTalkerStop(std::istream& is)
{
  MsgTalkerStop msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgTalkerStop\n";
    disconnect();
    return;
  }
  cout << name() << ": Talker stop on TG #" << msg.tg() << ": "
       << msg.callsign() << endl;

  std::ostringstream ss;
  ss << "talker_stop " << msg.tg() << " " << msg.callsign();
  processEvent(ss.str());
} /* ReflectorLogic::handleMsgTalkerStop */


void ReflectorLogic::handleMsgRequestQsy(std::istream& is)
{
  MsgRequestQsy msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgRequestQsy\n";
    disconnect();
    return;
  }
  cout << name() << ": Server QSY request for TG #" << msg.tg() << endl;
  if (m_tg_local_activity)
  {
    selectTg(msg.tg(), "tg_qsy", true);
  }
  else
  {
    m_last_qsy = msg.tg();
    selectTg(0, "", false);
    std::ostringstream os;
    if (m_qsy_pending_timer.timeout() > 0)
    {
      cout << name() << ": Server QSY request pending" << endl;
      os << "tg_qsy_pending " << msg.tg();
      m_qsy_pending_timer.setEnable(true);
      m_use_prio = false;
      m_tg_select_timeout_cnt = 1 + m_qsy_pending_timer.timeout() / 1000;
    }
    else
    {
      cout << name()
           << ": Server QSY request ignored due to no local activity" << endl;
      os << "tg_qsy_ignored " << msg.tg();
      m_use_prio = true;
      m_tg_select_timeout_cnt = 0;
    }
    processEvent(os.str());
  }
} /* ReflectorLogic::handleMsgRequestQsy */


void ReflectorLogic::handlMsgStartUdpEncryption(std::istream& is)
{
  //std::cout << "### ReflectorLogic::handlMsgStartUdpEncryption" << std::endl;

  if (m_con_state != STATE_EXPECT_START_UDP_ENCRYPTION)
  {
    std::cerr << "*** ERROR[" << name()
              << "]: Unexpected MsgStartUdpEncryption message" << std::endl;
    disconnect();
    return;
  }

  MsgStartUdpEncryption msg;
  if (!msg.unpack(is))
  {
    std::cerr << "*** ERROR[" << name()
              << "]: Could not unpack MsgStartUdpEncryption" << std::endl;
    disconnect();
    return;
  }
  m_con_state = STATE_EXPECT_UDP_HEARTBEAT;
  sendUdpRegisterMsg();
} /* ReflectorLogic::handlMsgStartUdpEncryption */


void ReflectorLogic::sendMsg(const ReflectorMsg& msg)
{
  if (!isConnected())
  {
    return;
  }

  m_tcp_heartbeat_tx_cnt = TCP_HEARTBEAT_TX_CNT_RESET;

  ostringstream ss;
  ReflectorMsg header(msg.type());
  if (!header.pack(ss) || !msg.pack(ss))
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to pack reflector TCP message\n";
    disconnect();
    return;
  }
  if (m_con.write(ss.str().data(), ss.str().size()) == -1)
  {
    std::cerr << "*** ERROR[" << name()
              << "]: Failed to write message to network connection"
              << std::endl;
    disconnect();
  }
} /* ReflectorLogic::sendMsg */


void ReflectorLogic::sendEncodedAudio(const void *buf, int count)
{
  if (!isLoggedIn())
  {
    return;
  }

  if (m_flush_timeout_timer.isEnabled())
  {
    m_flush_timeout_timer.setEnable(false);
  }
  sendUdpMsg(MsgUdpAudio(buf, count));
} /* ReflectorLogic::sendEncodedAudio */


void ReflectorLogic::flushEncodedAudio(void)
{
  if (!isLoggedIn())
  {
    flushTimeout();
    return;
  }
  sendUdpMsg(MsgUdpFlushSamples());
  m_flush_timeout_timer.setEnable(true);
} /* ReflectorLogic::flushEncodedAudio */


bool ReflectorLogic::udpCipherDataReceived(const IpAddress& addr, uint16_t port,
                                           void *buf, int count)
{
  if (static_cast<size_t>(count) < UdpCipher::AADLEN)
  {
    std::cout << "### ReflectorLogic::udpCipherDataReceived: Datagram too "
                 "short to hold associated data" << std::endl;
    return true;
  }
  stringstream ss;
  ss.write(reinterpret_cast<const char *>(buf), UdpCipher::AADLEN);
  if (!m_aad.unpack(ss))
  {
    std::cout << "*** WARNING: Unpacking associated data failed for UDP "
                 "datagram from " << addr << ":" << port << std::endl;
    return true;
  }
  //std::cout << "### ReflectorLogic::udpCipherDataReceived: m_aad.iv_cntr="
  //          << m_aad.iv_cntr << std::endl;
  m_udp_sock->setCipherIV(UdpCipher::IV{m_udp_cipher_iv_rand, 0,
                                        m_aad.iv_cntr});
  return false;
} /* ReflectorLogic::udpCipherDataReceived */


void ReflectorLogic::udpDatagramReceived(const IpAddress& addr, uint16_t port,
                                         void* aad, void *buf, int count)
{
  if (!isTcpLoggedIn())
  {
    return;
  }

  if (aad != nullptr)
  {
    //std::cout << "### ReflectorLogic::udpDatagramReceived: m_aad.iv_cntr="
    //          << m_aad.iv_cntr << std::endl;
  }

  if (addr != m_con.remoteHost())
  {
    cout << "*** WARNING[" << name()
         << "]: UDP packet received from wrong source address "
         << addr << ". Should be " << m_con.remoteHost() << "." << endl;
    return;
  }
  if (port != m_con.remotePort())
  {
    cout << "*** WARNING[" << name()
         << "]: UDP packet received with wrong source port number "
         << port << ". Should be " << m_con.remotePort() << "." << endl;
    return;
  }

  stringstream ss;
  ss.write(reinterpret_cast<const char *>(buf), count);

  ReflectorUdpMsg header;
  if (!header.unpack(ss))
  {
    cout << "*** WARNING[" << name()
         << "]: Unpacking failed for UDP message header" << endl;
    return;
  }

  //if (header.clientId() != m_client_id)
  //{
  //  cout << "*** WARNING[" << name()
  //       << "]: UDP packet received with wrong client id "
  //       << header.clientId() << ". Should be " << m_client_id << "." << endl;
  //  return;
  //}

    // Check sequence number
  if (m_aad.iv_cntr < m_next_udp_rx_seq) // Frame out of sequence (ignore)
  {
    std::cout << name()
              << ": Dropping out of sequence UDP frame with seq="
              << m_aad.iv_cntr << std::endl;
    return;
  }
  else if (m_aad.iv_cntr > m_next_udp_rx_seq) // Frame lost
  {
    std::cout << name() << ": UDP frame(s) lost. Expected seq="
              << m_next_udp_rx_seq
              << " but received " << m_aad.iv_cntr
              << ". Resetting next expected sequence number to "
              << (m_aad.iv_cntr + 1) << std::endl;
  }
  m_next_udp_rx_seq = m_aad.iv_cntr + 1;

  m_udp_heartbeat_rx_cnt = UDP_HEARTBEAT_RX_CNT_RESET;

  if ((m_con_state == STATE_EXPECT_UDP_HEARTBEAT) &&
      (header.type() == MsgUdpHeartbeat::TYPE))
  {
    std::cout << name() << ": Bidirectional UDP communication verified"
              << std::endl;
    m_con.markAsEstablished();
    m_con_state = STATE_CONNECTED;

    if (m_selected_tg > 0)
    {
      cout << name() << ": Selecting TG #" << m_selected_tg << endl;
      sendMsg(MsgSelectTG(m_selected_tg));
    }

    if (!m_monitor_tgs.empty())
    {
      sendMsg(MsgTgMonitor(
            std::set<uint32_t>(m_monitor_tgs.begin(), m_monitor_tgs.end())));
    }
  }

  if (!isLoggedIn())
  {
    return;
  }

  switch (header.type())
  {
    case MsgUdpHeartbeat::TYPE:
      break;

    case MsgUdpAudio::TYPE:
    {
      MsgUdpAudio msg;
      if (!msg.unpack(ss))
      {
        cerr << "*** WARNING[" << name() << "]: Could not unpack MsgUdpAudio\n";
        return;
      }
      if (!msg.audioData().empty())
      {
        gettimeofday(&m_last_talker_timestamp, NULL);
        m_dec->writeEncodedSamples(
            &msg.audioData().front(), msg.audioData().size());
      }
      break;
    }

    case MsgUdpFlushSamples::TYPE:
      m_dec->flushEncodedSamples();
      timerclear(&m_last_talker_timestamp);
      break;

    case MsgUdpAllSamplesFlushed::TYPE:
      m_enc->allEncodedSamplesFlushed();
      break;

    default:
      // Better ignoring unknown protocol messages for easier addition of new
      // messages while still being backwards compatible

      //cerr << "*** WARNING[" << name()
      //     << "]: Unknown UDP protocol message received: msg_type="
      //     << header.type() << endl;
      break;
  }
} /* ReflectorLogic::udpDatagramReceived */

void ReflectorLogic::sendUdpMsg(const UdpCipher::AAD& aad,
                                const ReflectorUdpMsg& msg)
{
  m_udp_heartbeat_tx_cnt = m_udp_heartbeat_tx_cnt_reset;

  if (m_udp_sock == 0)
  {
    return;
  }

  ReflectorUdpMsg header(msg.type());
  ostringstream ss;
  if (!header.pack(ss) || !msg.pack(ss))
  {
    std::cerr << "*** ERROR[" << name()
              << "]: Failed to pack reflector UDP message" << std::endl;
    return;
  }
  m_udp_sock->setCipherIV(UdpCipher::IV{m_udp_cipher_iv_rand, m_client_id,
                                        aad.iv_cntr});
  std::ostringstream adss;
  if (!aad.pack(adss))
  {
    std::cout << "*** WARNING: Packing associated data failed for UDP "
                 "datagram to " << m_con.remoteHost() << ":"
              << m_con.remotePort() << std::endl;
    return;
  }
  m_udp_sock->write(m_con.remoteHost(), m_con.remotePort(),
                    adss.str().data(), adss.str().size(),
                    ss.str().data(), ss.str().size());
} /* ReflectorLogic::sendUdpMsg */


void ReflectorLogic::sendUdpMsg(const ReflectorUdpMsg& msg)
{
  if (!isLoggedIn())
  {
    return;
  }
  sendUdpMsg(UdpCipher::AAD{m_udp_cipher_iv_cntr++}, msg);
} /* ReflectorLogic::sendUdpMsg */


void ReflectorLogic::sendUdpRegisterMsg(void)
{
  sendUdpMsg(UdpCipher::InitialAAD{m_client_id}, MsgUdpHeartbeat());
} /* ReflectorLogic::sendUdpRegisterMsg */


void ReflectorLogic::connect(void)
{
  if (!isConnected())
  {
    m_reconnect_timer.setEnable(false);
    std::cout << name() << ": Connecting to service " << m_con.service()
              << std::endl;
    m_con.connect();
    m_con.setSslContext(m_ssl_ctx, false);
  }
} /* ReflectorLogic::connect */


void ReflectorLogic::disconnect(void)
{
  bool was_connected = m_con.isConnected();
  m_con.disconnect();
  if (was_connected)
  {
    onDisconnected(&m_con, TcpConnection::DR_ORDERED_DISCONNECT);
  }
  m_con_state = STATE_DISCONNECTED;
} /* ReflectorLogic::disconnect */


void ReflectorLogic::reconnect(void)
{
  disconnect();
  connect();
} /* ReflectorLogic::reconnect */


bool ReflectorLogic::isConnected(void) const
{
  return m_con.isConnected();
} /* ReflectorLogic::isConnected */


void ReflectorLogic::allEncodedSamplesFlushed(void)
{
  sendUdpMsg(MsgUdpAllSamplesFlushed());
} /* ReflectorLogic::allEncodedSamplesFlushed */


void ReflectorLogic::flushTimeout(Async::Timer *t)
{
  m_flush_timeout_timer.setEnable(false);
  m_enc->allEncodedSamplesFlushed();
} /* ReflectorLogic::flushTimeout */


void ReflectorLogic::handleTimerTick(Async::Timer *t)
{
  if (timerisset(&m_last_talker_timestamp))
  {
    struct timeval now, diff;
    gettimeofday(&now, NULL);
    timersub(&now, &m_last_talker_timestamp, &diff);
    if (diff.tv_sec > 3)
    {
      cout << name() << ": Last talker audio timeout" << endl;
      m_dec->flushEncodedSamples();
      timerclear(&m_last_talker_timestamp);
    }
  }

  if (--m_udp_heartbeat_tx_cnt == 0)
  {
    if (m_con_state == STATE_EXPECT_UDP_HEARTBEAT)
    {
      sendUdpRegisterMsg();
    }
    else if (isLoggedIn())
    {
      sendUdpMsg(MsgUdpHeartbeat());
    }
  }

  if (--m_tcp_heartbeat_tx_cnt == 0)
  {
    sendMsg(MsgHeartbeat());
  }

  if (--m_udp_heartbeat_rx_cnt == 0)
  {
    cout << name() << ": UDP Heartbeat timeout" << endl;
    disconnect();
  }

  if (--m_tcp_heartbeat_rx_cnt == 0)
  {
    cout << name() << ": Heartbeat timeout" << endl;
    disconnect();
  }
} /* ReflectorLogic::handleTimerTick */


bool ReflectorLogic::setAudioCodec(const std::string& codec_name)
{
  delete m_enc;
  m_enc = Async::AudioEncoder::create(codec_name);
  if (m_enc == 0)
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to initialize " << codec_name
         << " audio encoder" << endl;
    m_enc = Async::AudioEncoder::create("DUMMY");
    assert(m_enc != 0);
    return false;
  }
  m_enc->writeEncodedSamples.connect(
      mem_fun(*this, &ReflectorLogic::sendEncodedAudio));
  m_enc->flushEncodedSamples.connect(
      mem_fun(*this, &ReflectorLogic::flushEncodedAudio));
  m_enc_endpoint->registerSink(m_enc, false);

  string opt_prefix(m_enc->name());
  opt_prefix += "_ENC_";
  list<string> names = cfg().listSection(name());
  for (list<string>::const_iterator nit=names.begin(); nit!=names.end(); ++nit)
  {
    if ((*nit).find(opt_prefix) == 0)
    {
      string opt_value;
      cfg().getValue(name(), *nit, opt_value);
      string opt_name((*nit).substr(opt_prefix.size()));
      m_enc->setOption(opt_name, opt_value);
    }
  }
  m_enc->printCodecParams();

  AudioSink *sink = 0;
  if (m_dec != 0)
  {
    sink = m_dec->sink();
    m_dec->unregisterSink();
    delete m_dec;
  }
  m_dec = Async::AudioDecoder::create(codec_name);
  if (m_dec == 0)
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to initialize " << codec_name
         << " audio decoder" << endl;
    m_dec = Async::AudioDecoder::create("DUMMY");
    assert(m_dec != 0);
    return false;
  }
  m_dec->allEncodedSamplesFlushed.connect(
      mem_fun(*this, &ReflectorLogic::allEncodedSamplesFlushed));
  if (sink != 0)
  {
    m_dec->registerSink(sink, true);
  }

  opt_prefix = string(m_dec->name()) + "_DEC_";
  names = cfg().listSection(name());
  for (list<string>::const_iterator nit=names.begin(); nit!=names.end(); ++nit)
  {
    if ((*nit).find(opt_prefix) == 0)
    {
      string opt_value;
      cfg().getValue(name(), *nit, opt_value);
      string opt_name((*nit).substr(opt_prefix.size()));
      m_dec->setOption(opt_name, opt_value);
    }
  }
  m_dec->printCodecParams();

  return true;
} /* ReflectorLogic::setAudioCodec */


bool ReflectorLogic::codecIsAvailable(const std::string &codec_name)
{
  return AudioEncoder::isAvailable(codec_name) &&
         AudioDecoder::isAvailable(codec_name);
} /* ReflectorLogic::codecIsAvailable */


void ReflectorLogic::onLogicConInStreamStateChanged(bool is_active,
                                                    bool is_idle)
{
  //cout << "### ReflectorLogic::onLogicConInStreamStateChanged: is_active="
  //     << is_active << "  is_idle=" << is_idle << endl;
  if (is_idle)
  {
    if (m_qsy_pending_timer.isEnabled())
    {
      std::ostringstream os;
      os << "tg_qsy_on_sql " << m_last_qsy;
      processEvent(os.str());
      selectTg(m_last_qsy, "", true);
      m_qsy_pending_timer.setEnable(false);
      m_tg_local_activity = true;
      m_use_prio = false;
    }
  }
  else
  {
    if ((m_logic_con_in_valve != 0) && m_tg_local_activity)
    {
      m_logic_con_in_valve->setOpen(true);
    }
    if (m_tg_select_timeout_cnt == 0) // No TG currently selected
    {
      if (m_default_tg > 0)
      {
        selectTg(m_default_tg, "tg_default_activation", !m_mute_first_tx_loc);
      }
    }
    m_qsy_pending_timer.reset();
    m_tg_local_activity = true;
    m_use_prio = false;
    m_tg_select_timeout_cnt =
      (m_selected_tg > 0) ? m_tg_select_timeout : m_tg_select_inhibit_timeout;
  }

  if (!m_tg_selection_event.empty())
  {
    //processTgSelectionEvent();
    m_report_tg_timer.reset();
    m_report_tg_timer.setEnable(true);
  }

  checkIdle();
} /* ReflectorLogic::onLogicConInStreamStateChanged */


void ReflectorLogic::onLogicConOutStreamStateChanged(bool is_active,
                                                     bool is_idle)
{
  //cout << "### ReflectorLogic::onLogicConOutStreamStateChanged: is_active="
  //     << is_active << "  is_idle=" << is_idle << endl;
  if (!is_idle && (m_tg_select_timeout_cnt > 0))
  {
    m_tg_select_timeout_cnt = m_tg_select_timeout;
  }

  if (!m_tg_selection_event.empty())
  {
    //processTgSelectionEvent();
    m_report_tg_timer.reset();
    m_report_tg_timer.setEnable(true);
  }

  checkIdle();
} /* ReflectorLogic::onLogicConOutStreamStateChanged */


void ReflectorLogic::tgSelectTimerExpired(void)
{
  //cout << "### ReflectorLogic::tgSelectTimerExpired: m_tg_select_timeout_cnt="
  //     << m_tg_select_timeout_cnt << endl;
  if (m_tg_select_timeout_cnt > 0)
  {
    if (m_logic_con_out->isIdle() && m_logic_con_in->isIdle() &&
        (--m_tg_select_timeout_cnt == 0))
    {
      selectTg(0, "tg_selection_timeout", false);
    }
  }
} /* ReflectorLogic::tgSelectTimerExpired */


void ReflectorLogic::selectTg(uint32_t tg, const std::string& event, bool unmute)
{
  cout << name() << ": Selecting TG #" << tg << endl;

  m_tg_selection_event.clear();
  if (!event.empty())
  {
    ostringstream os;
    os << event << " " << tg << " " << m_selected_tg;
    m_tg_selection_event = os.str();
    m_report_tg_timer.reset();
    m_report_tg_timer.setEnable(true);
  }

  if (tg != m_selected_tg)
  {
    sendMsg(MsgSelectTG(tg));
    if (m_selected_tg != 0)
    {
      m_previous_tg = m_selected_tg;
    }
    m_selected_tg = tg;
    if (tg == 0)
    {
      m_tg_local_activity = false;
      m_use_prio = true;
    }
    else
    {
      m_tg_local_activity = !m_logic_con_in->isIdle();
      m_qsy_pending_timer.setEnable(false);
    }
    m_event_handler->setVariable(name() + "::selected_tg", m_selected_tg);
    m_event_handler->setVariable(name() + "::previous_tg", m_previous_tg);

    ostringstream os;
    os << "tg_selected " << m_selected_tg << " " << m_previous_tg;
    processEvent(os.str());
  }
  m_tg_select_timeout_cnt = (tg > 0) ? m_tg_select_timeout : 0;

  if (m_logic_con_in_valve != 0)
  {
    m_logic_con_in_valve->setOpen(unmute);
  }
} /* ReflectorLogic::selectTg */


void ReflectorLogic::processEvent(const std::string& event)
{
  m_event_handler->processEvent(name() + "::" + event);
  checkIdle();
} /* ReflectorLogic::processEvent */


void ReflectorLogic::processTgSelectionEvent(void)
{
  if (!m_logic_con_out->isIdle() || !m_logic_con_in->isIdle() ||
      m_tg_selection_event.empty())
  {
    return;
  }
  processEvent(m_tg_selection_event);
  m_tg_selection_event.clear();
} /* ReflectorLogic::processTgSelectionEvent */


void ReflectorLogic::checkTmpMonitorTimeout(void)
{
  bool changed = false;
  MonitorTgsSet::iterator it = m_monitor_tgs.begin();
  while (it != m_monitor_tgs.end())
  {
    MonitorTgsSet::iterator next=it;
    ++next;
    const MonitorTgEntry& mte = *it;
    if (mte.timeout > 0)
    {
        // NOTE: mte.timeout is mutable
      if (--mte.timeout <= 0)
      {
        std::cout << name() << ": Temporary monitor timeout for TG #"
                  << mte.tg << std::endl;
        changed = true;
        m_monitor_tgs.erase(it);
        std::ostringstream os;
        os << "tmp_monitor_remove " << mte.tg;
        processEvent(os.str());
      }
    }
    it = next;
  }
  if (changed)
  {
    sendMsg(MsgTgMonitor(std::set<uint32_t>(
            m_monitor_tgs.begin(), m_monitor_tgs.end())));
  }
} /* ReflectorLogic::checkTmpMonitorTimeout */


void ReflectorLogic::qsyPendingTimeout(void)
{
  m_qsy_pending_timer.setEnable(false);
  m_use_prio = true;
  m_tg_select_timeout_cnt = 0;
  cout << name()
       << ": Server QSY request ignored due to no local activity" << endl;
  std::ostringstream os;
  os << "tg_qsy_ignored " << m_last_qsy;
  processEvent(os.str());
} /* ReflectorLogic::qsyPendingTimeout */


bool ReflectorLogic::isIdle(void)
{
  return m_logic_con_out->isIdle() && m_logic_con_in->isIdle();
} /* ReflectorLogic::isIdle */


void ReflectorLogic::checkIdle(void)
{
  setIdle(isIdle());
} /* ReflectorLogic::checkIdle */


void ReflectorLogic::handlePlayFile(const std::string& path)
{
  setIdle(false);
  LinkManager::instance()->playFile(this, path);
} /* ReflectorLogic::handlePlayFile */


void ReflectorLogic::handlePlaySilence(int duration)
{
  setIdle(false);
  LinkManager::instance()->playSilence(this, duration);
} /* ReflectorLogic::handlePlaySilence */


void ReflectorLogic::handlePlayTone(int fq, int amp, int duration)
{
  setIdle(false);
  LinkManager::instance()->playTone(this, fq, amp, duration);
} /* ReflectorLogic::handlePlayTone */


void ReflectorLogic::handlePlayDtmf(const std::string& digit, int amp,
                                    int duration)
{
  setIdle(false);
  LinkManager::instance()->playDtmf(this, digit, amp, duration);
} /* ReflectorLogic::handlePlayDtmf */


bool ReflectorLogic::loadClientCertificate(void)
{
  if (m_ssl_cert.readPemFile(m_crtfile) &&
      !m_ssl_cert.isNull() &&
      //cert.verify(m_ssl_pkey) &&
      m_ssl_cert.timeIsWithinRange())
  {
    if (!m_ssl_ctx.setCertificateFiles(m_keyfile, m_crtfile))
    {
      std::cerr << "*** ERROR: Failed to read and verify key ('"
                << m_keyfile << "') and certificate ('"
                << m_crtfile << "') files in logic \"" << name() << "'. "
                << "If key- and cert-file does not match, the certificate "
                   "has expired, or is invalid for any other reason, you "
                   "need to remove the cert file in order to trigger the "
                   "generation of a new one signed by the SvxReflector "
                   "manager. If there is an access problem you need to fix "
                   "the permissions of the key- and certificate files."
                << std::endl;
      return false;
    }
  }
  return true;
} /* ReflectorLogic::loadClientCertificate */


void ReflectorLogic::csrAddSubjectNamesFromConfig(void)
{
  const std::string prefix = "CERT_SUBJ_";
  for (const auto& section : cfg().listSection(name()))
  {
    const std::string sname = section.substr(prefix.size());
    std::string value;
    if ((section.rfind(prefix, 0) == 0) &&
        cfg().getValue(name(), prefix + sname, value) &&
        !value.empty())
    {
      if (!m_ssl_csr.addSubjectName(sname, value))
      {
        std::cerr << "*** WARNING: Failed to set subject name '" << sname
                  << "' in certificate signing request." << std::endl;
      }
    }
  }
} /* ReflectorLogic::csrAddSubjectName */


/*
 * This file has not been truncated
 */
