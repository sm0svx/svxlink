/**
@file	 AsyncCppDnsLookupWorker.cpp
@brief   Contains a class to execute DNS queries in the Posix environment
@author  Tobias Blomberg
@date	 2003-04-17

This file contains a class for executing DNS quries in the Cpp variant of
the async environment. This class should never be used directly. It is
used by Async::CppApplication to execute DNS queries.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2025 Tobias Blomberg

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
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>

#include <cassert>
#include <cstring>
#include <mutex>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncDnsLookup.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncCppDnsLookupWorker.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

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

namespace {
#if !(__RES >= 19991006)
  std::mutex res_mutex;
#endif

  class Resolver
  {
    public:
      Resolver(void) { m_state.options = RES_DEFAULT; }
      ~Resolver(void) { close(); }
#if __RES >= 19991006
      int init(void)
      {
        return res_ninit(&m_state);
      }
      int search(const char *dname, int dclass, int type,
                 unsigned char* answer, int anslen)
      {
        return res_nsearch(&m_state, dname, dclass, type, answer, anslen);
      }
      void close(void)
      {
          // FIXME: Valgrind complain about leaked memory in the resolver
          //        library when a lookup fails. It seems to be a one time leak
          //        though so it does not grow with every failed lookup. But
          //        even so, it seems that res_nclose is not cleaning up
          //        properly.  Glibc 2.33-18 on Fedora 34.
        res_nclose(&m_state);
      }
#else
      int init(void)
      {
        const std::lock_guard<std::mutex> lock(res_mutex);
        int rc = res_init();
        memcpy(&m_state, &_res, sizeof(m_state));
        return rc;
      }
      int search(const char *dname, int dclass, int type,
                 unsigned char* answer, int anslen)
      {
        const std::lock_guard<std::mutex> lock(res_mutex);
        struct __res_state old_state;
        memcpy(&old_state, &_res, sizeof(old_state));
        memcpy(&_res, &m_state, sizeof(m_state));
        int rc = res_search(dname, dclass, type, answer, anslen);
        memcpy(&_res, &old_state, sizeof(old_state));
        return rc;
      }
      void close(void) {}
#endif
    private:
      struct __res_state m_state;
  };
};


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



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


CppDnsLookupWorker::CppDnsLookupWorker(const DnsLookup& dns)
  : DnsLookupWorker(dns)
{
  m_notifier_watch.activity.connect(
      sigc::mem_fun(*this, &CppDnsLookupWorker::notificationReceived));
} /* CppDnsLookupWorker::CppDnsLookupWorker */


CppDnsLookupWorker::~CppDnsLookupWorker(void)
{
  abortLookup();
} /* CppDnsLookupWorker::~CppDnsLookupWorker */


DnsLookupWorker& CppDnsLookupWorker::operator=(DnsLookupWorker&& other_base)
{
  this->DnsLookupWorker::operator=(std::move(other_base));
  auto& other = static_cast<CppDnsLookupWorker&>(other_base);

  abortLookup();

  m_result = std::move(other.m_result);
  assert(!other.m_result.valid());

  m_notifier_watch = std::move(other.m_notifier_watch);

  return *this;
} /* CppDnsLookupWorker::operator=(DnsLookupWorker&&) */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

bool CppDnsLookupWorker::doLookup(void)
{
    // A lookup is already running
  if (m_result.valid())
  {
    return true;
  }

  setLookupFailed(false);

  int fd[2];
  if (pipe(fd) != 0)
  {
    printErrno("ERROR: Could not create pipe");
    setLookupFailed();
    return false;
  }
  m_notifier_watch.setFd(fd[0], FdWatch::FD_WATCH_RD);
  m_notifier_watch.setEnabled(true);

  m_ctx = std::unique_ptr<ThreadContext>(new ThreadContext);
  m_ctx->label = dns().label();
  m_ctx->type = dns().type();
  m_ctx->notifier_wr = fd[1];
  m_ctx->anslen = 0;
  m_ctx->thread_cerr.clear();
  m_result = std::async(std::launch::async, workerFunc, std::ref(*m_ctx));

  return true;

} /* CppDnsLookupWorker::doLookup */


void CppDnsLookupWorker::abortLookup(void)
{
  if (m_result.valid())
  {
    m_result.get();
  }

  int fd = m_notifier_watch.fd();
  if (fd >= 0)
  {
    m_notifier_watch.setFd(-1, FdWatch::FD_WATCH_RD);
    close(fd);
  }

  m_ctx.reset();
} /* CppDnsLookupWorker::abortLookup */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

/*
 *----------------------------------------------------------------------------
 * Method:    CppDnsLookupWorker::workerFunc
 * Purpose:   This is the function that do the actual DNS lookup. It is
 *    	      started as a separate thread since res_nsearch is a
 *    	      blocking function.
 * Input:     ctx - A context containing query and result parameters
 * Output:    The answer and anslen variables in the ThreadContext will be
 *            filled in with the lookup result. The context will be returned
 *            from this function.
 * Author:    Tobias Blomberg
 * Created:   2021-07-14
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void CppDnsLookupWorker::workerFunc(CppDnsLookupWorker::ThreadContext& ctx)
{
  std::ostream& th_cerr = ctx.thread_cerr;

  int qtype = 0;
  switch (ctx.type)
  {
    case DnsLookup::Type::A:
    {
      struct addrinfo hints = {0};
      hints.ai_family = AF_INET;
      int ret = getaddrinfo(ctx.label.c_str(), NULL, &hints, &ctx.addrinfo);
      if (ret != 0)
      {
        th_cerr << "*** WARNING[getaddrinfo]: Could not look up host \""
                << ctx.label << "\": " << gai_strerror(ret) << std::endl;
      }
      else if (ctx.addrinfo == nullptr)
      {
        th_cerr << "*** WARNING[getaddrinfo]: No address info returned "
                   "for host \"" << ctx.label << "\"" << std::endl;
      }
      break;
    }
    case DnsLookup::Type::PTR:
    {
      IpAddress ip_addr;
      size_t arpa_domain_pos = ctx.label.find(".in-addr.arpa");
      if (arpa_domain_pos != std::string::npos)
      {
        ip_addr.setIpFromString(ctx.label.substr(0, arpa_domain_pos));
        struct in_addr addr = ip_addr.ip4Addr();
        addr.s_addr = htonl(addr.s_addr);
        ip_addr.setIp(addr);
      }
      else
      {
        ip_addr.setIpFromString(ctx.label);
      }
      if (!ip_addr.isEmpty())
      {
        struct sockaddr_in in_addr = {0};
        in_addr.sin_family = AF_INET;
        in_addr.sin_addr = ip_addr.ip4Addr();
        int ret = getnameinfo(reinterpret_cast<struct sockaddr*>(&in_addr),
                              sizeof(in_addr),
                              ctx.host, sizeof(ctx.host),
                              NULL, 0, NI_NAMEREQD);
        if (ret != 0)
        {
          th_cerr << "*** WARNING[getnameinfo]: Could not look up IP \""
                  << ctx.label << "\": " << gai_strerror(ret) << std::endl;
        }
      }
      else
      {
        th_cerr << "*** WARNING: Failed to parse PTR label \""
                << ctx.label << "\"" << std::endl;
      }
      break;
    }
    case DnsLookup::Type::CNAME:
      qtype = ns_t_cname;
      break;
    case DnsLookup::Type::SRV:
      qtype = ns_t_srv;
      break;
    default:
      assert(0);
  }

  if (qtype != 0)
  {
    Resolver res;
    int ret = res.init();
    if (ret != -1)
    {
      const char *dname = ctx.label.c_str();
      ctx.anslen = res.search(dname, ns_c_in, qtype,
                              ctx.answer, sizeof(ctx.answer));
      if (ctx.anslen == -1)
      {
        th_cerr << "*** ERROR: Name resolver failure -- res_nsearch: "
                << hstrerror(h_errno) << std::endl;
      }
    }
    else
    {
      th_cerr << "*** ERROR: Name resolver failure -- res_ninit: "
              << hstrerror(h_errno) << std::endl;
    }
  }

  close(ctx.notifier_wr);
  ctx.notifier_wr = -1;
} /* CppDnsLookupWorker::workerFunc */


/*
 *----------------------------------------------------------------------------
 * Method:    CppDnsLookupWorker::notificationReceived
 * Purpose:   When the DNS lookup thread is done, this function will be
 *            called to parse the result and notify the user that an answer
 *            is available.
 * Input:     w - The file watch object (notification pipe)
 * Output:    None
 * Author:    Tobias Blomberg
 * Created:   2005-04-12
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void CppDnsLookupWorker::notificationReceived(FdWatch *w)
{
  char buf[1];
  int cnt = read(w->fd(), buf, sizeof(buf));
  assert(cnt == 0);
  close(w->fd());
  w->setFd(-1, FdWatch::FD_WATCH_RD);

  m_result.get();

  const std::string& thread_errstr = m_ctx->thread_cerr.str();
  if (!thread_errstr.empty())
  {
    std::cerr << thread_errstr;
    setLookupFailed();
  }

  if (m_ctx->type == DnsResourceRecord::Type::A)
  {
    if (m_ctx->addrinfo != nullptr)
    {
      struct addrinfo *entry;
      std::vector<IpAddress> the_addresses;
      for (entry = m_ctx->addrinfo; entry != 0; entry = entry->ai_next)
      {
        IpAddress ip_addr(
            reinterpret_cast<struct sockaddr_in*>(entry->ai_addr)->sin_addr);
        //std::cout << "### ai_family=" << entry->ai_family
        //          << "  ai_socktype=" << entry->ai_socktype
        //          << "  ai_protocol=" << entry->ai_protocol
        //          << "  ip=" << ip_addr << std::endl;
        if (find(the_addresses.begin(), the_addresses.end(), ip_addr) ==
            the_addresses.end())
        {
          the_addresses.push_back(ip_addr);
          addResourceRecord(
              new DnsResourceRecordA(m_ctx->label, 0, ip_addr));
        }
      }
      m_ctx.reset();
    }
  }
  else if (m_ctx->type == DnsResourceRecord::Type::PTR)
  {
    if (m_ctx->host[0] != '\0')
    {
      addResourceRecord(
          new DnsResourceRecordPTR(m_ctx->label, 0, m_ctx->host));
    }
  }
  else
  {
    if (m_ctx->anslen == -1)
    {
      workerDone();
      return;
    }

    ns_msg msg;
    int ret = ns_initparse(m_ctx->answer, m_ctx->anslen, &msg);
    if (ret == -1)
    {
      std::stringstream ss;
      ss << "WARNING: ns_initparse failed (anslen=" << m_ctx->anslen << ")";
      printErrno(ss.str());
      setLookupFailed();
      workerDone();
      return;
    }

    uint16_t msg_cnt = ns_msg_count(msg, ns_s_an);
    if (msg_cnt == 0)
    {
      setLookupFailed();
    }
    for (uint16_t rrnum=0; rrnum<msg_cnt; ++rrnum)
    {
      ns_rr rr;
      ret = ns_parserr(&msg, ns_s_an, rrnum, &rr);
      if (ret == -1)
      {
        printErrno("WARNING: DNS lookup failure in ns_parserr");
        setLookupFailed();
        continue;
      }
      const char *name = ns_rr_name(rr);
      uint16_t rr_class = ns_rr_class(rr);
      if (rr_class != ns_c_in)
      {
        std::cerr << "*** WARNING: Wrong RR class in DNS answer: "
                  << rr_class << std::endl;
        setLookupFailed();
        continue;
      }
      uint32_t ttl = ns_rr_ttl(rr);
      uint16_t type = ns_rr_type(rr);
      const unsigned char *cp = ns_rr_rdata(rr);
      switch (type)
      {
        case ns_t_a:
        {
          struct in_addr in_addr;
          uint32_t ip = ns_get32(cp);
          in_addr.s_addr = ntohl(ip);
          addResourceRecord(
              new DnsResourceRecordA(name, ttl, IpAddress(in_addr)));
          break;
        }

        case ns_t_ptr:
        {
          char exp_dn[NS_MAXDNAME+1];
          ret = ns_name_uncompress(ns_msg_base(msg), ns_msg_end(msg), cp,
                                   exp_dn, NS_MAXDNAME);
          if (ret == -1)
          {
            printErrno("WARNING: DNS lookup failure in ns_name_uncompress");
            setLookupFailed();
            continue;
          }
          size_t exp_dn_len = strlen(exp_dn);
          exp_dn[exp_dn_len] = '.';
          exp_dn[exp_dn_len+1] = 0;
          addResourceRecord(new DnsResourceRecordPTR(name, ttl, exp_dn));
          break;
        }

        case ns_t_cname:
        {
          char exp_dn[NS_MAXDNAME+1];
          ret = ns_name_uncompress(ns_msg_base(msg), ns_msg_end(msg), cp,
              exp_dn, NS_MAXDNAME);
          if (ret == -1)
          {
            printErrno("WARNING: DNS lookup failure in ns_name_uncompress");
            setLookupFailed();
            continue;
          }
          size_t exp_dn_len = strlen(exp_dn);
          exp_dn[exp_dn_len] = '.';
          exp_dn[exp_dn_len+1] = 0;
          addResourceRecord(new DnsResourceRecordCNAME(name, ttl, exp_dn));
          break;
        }

        case ns_t_srv:
        {
          unsigned int prio = ns_get16(cp);
          cp += NS_INT16SZ;
          unsigned int weight = ns_get16(cp);
          cp += NS_INT16SZ;
          unsigned int port = ns_get16(cp);
          cp += NS_INT16SZ;
          char exp_dn[NS_MAXDNAME+1];
          ret = ns_name_uncompress(ns_msg_base(msg), ns_msg_end(msg), cp,
              exp_dn, NS_MAXDNAME);
          if (ret == -1)
          {
            printErrno("WARNING: DNS lookup failure in ns_name_uncompress");
            setLookupFailed();
            continue;
          }
          size_t exp_dn_len = strlen(exp_dn);
          exp_dn[exp_dn_len] = '.';
          exp_dn[exp_dn_len+1] = 0;
          addResourceRecord(
              new DnsResourceRecordSRV(name, ttl, prio, weight, port, exp_dn));
          break;
        }

        default:
          std::cerr << "*** WARNING: Unsupported RR type, " << type
                    << ", received in DNS query for " << name << std::endl;
          setLookupFailed();
          break;
      }
    }
  }
  workerDone();
} /* CppDnsLookupWorker::notificationReceived */


void CppDnsLookupWorker::printErrno(const std::string& msg)
{
  static thread_local struct
  {
    char errstr[256];
    const char* operator()(int ret) { assert(ret == 0); return errstr; }
    const char* operator()(char* ret) { return ret; }
  } wrap;
  auto errmsg = wrap(strerror_r(errno, wrap.errstr, sizeof(wrap.errstr)));
  std::cerr << "*** " << msg << ": " << errmsg << std::endl;
} /* CppDnsLookupWorker::printErrno */


/*
 * This file has not been truncated
 */

