/**
@file   AsyncTcpPrioClientBase.cpp
@brief  Contains a base class for creating prioritized TCP client connections
@author Tobias Blomberg / SM0SVX
@date   2022-02-12

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2022 Tobias Blomberg / SM0SVX

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

#include <sys/time.h>
#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncApplication.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncTcpPrioClientBase.h"
//#define ASYNC_STATE_MACHINE_DEBUG
#include "AsyncStateMachine.h"


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

#ifdef ASYNC_STATE_MACHINE_DEBUG
#define DEBUG_EVENT \
  std::cout << "### StateMachine: " \
            << NAME << "(" << __func__ << ")" << std::endl
#else
#define DEBUG_EVENT
#endif


/****************************************************************************
 *
 * Static class variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

class TcpPrioClientBase::Machine
{
  public:
    Machine(TcpPrioClientBase *client)
      : ctx(client), m(&ctx)
    {
      ctx.dns.resultsReady.connect(
          [&](DnsLookup&)
          {
            m.state().dnsResultsReadyEvent();
          });
      ctx.bg_con->connected.connect(
          [&](void)
          {
            m.state().bgConnectedEvent();
          });
      ctx.bg_con->conObj()->disconnected.connect(
          [&](TcpConnection*, TcpConnection::DisconnectReason)
          {
            m.state().bgDisconnectedEvent();
          });
      m.start();
    }

    void setReconnectMinTime(unsigned t)
    {
      ctx.connect_retry_wait.setMinTime(t);
    }
    void setReconnectMaxTime(unsigned t)
    {
      ctx.connect_retry_wait.setMaxTime(t);
    }
    void setReconnectBackoffPercent(unsigned p)
    {
      ctx.connect_retry_wait.setBackoffPercent(p);
    }
    void setReconnectRandomizePercent(unsigned p)
    {
      ctx.connect_retry_wait.setRandomizePercent(p);
    }

    void setLookupParams(const std::string& label, DnsLookup::Type type)
    {
      ctx.dns.setLookupParams(label, DnsLookup::Type::SRV);
    }

    const std::string& label(void) const
    {
      return ctx.dns.label();
    }

    void addStaticResourceRecord(DnsResourceRecord* rr)
    {
      ctx.dns.addStaticResourceRecord(rr);
    }

    void connect(void)
    {
      m.state().connectEvent();
    }

    void disconnect(void)
    {
      m.state().disconnectEvent();
    }

    void markAsEstablished(void)
    {
      ctx.marked_as_established = true;
    }

    bool markedAsEstablished(void) const
    {
      return ctx.marked_as_established;
    }

    bool isIdle(void) const
    {
      return m.isActive<StateDisconnected>();
    }

    bool isPrimary(void) const
    {
      return m.isActive<StateConnectedHighestPrio>();
    }

    void connectionEstablished(void)
    {
      m.state().connectedEvent();
    }

    void onDisconnected(void)
    {
      m.state().disconnectedEvent();
    }

  private:
    class BackoffTime
    {
      public:
        using Time = unsigned;
        using Percent = unsigned;
        void setMinTime(Time t)
        {
          m_min_time = t;
          m_time = std::max(m_time, m_min_time);
        }
        void setMaxTime(Time t)
        {
          m_max_time = t;
          m_time = std::min(m_time, m_max_time);
        }
        void setBackoffPercent(Percent p) { m_backoff = p; }
        void setRandomizePercent(Percent p) { m_randomize = p; }
        void reset(void) { m_time = m_min_time; }
        void backoff(void)
        {
          m_time = std::min(
              m_time + std::max(m_time * m_backoff / 100, Time(1)),
              m_max_time);
        }
        Time time(void)
        {
          auto t = m_time;
          backoff();
          return t + std::rand() % std::max(t * m_randomize / 100, Time(1));
        }
        operator Time() { return time(); }

      private:
        Time    m_min_time  = 1000;
        Time    m_max_time  = 20000;
        Percent m_backoff   = 50;
        Percent m_randomize = 10;
        Time    m_time      = m_min_time;
    }; /* BackoffTime */


      // State machine context
    struct Context
    {
      using DnsSRVList = DnsLookup::SharedRRList<const DnsResourceRecordSRV>;

      TcpPrioClientBase*              client                  = nullptr;
      std::unique_ptr<TcpClientBase>  bg_con;
      DnsLookup                       dns;
      DnsSRVList                      rrs;
      DnsSRVList::iterator            next_rr                 = rrs.end();
      BackoffTime                     connect_retry_wait;
      bool                            marked_as_established   = false;

      Context(TcpPrioClientBase *client)
        : client(client), bg_con(client->newTcpClient()) {}

      void closeConnection(void)
      {
        client->closeConnection();
      }

      void emitDisconnected(TcpConnection::DisconnectReason reason)
      {
        client->emitDisconnected(reason);
      }

      void connect(const std::string &remote_host, uint16_t remote_port)
      {
        client->TcpClientBase::connect(remote_host, remote_port);
      }

      void emitConnected(void)
      {
        //client->emitConnected();
        client->TcpClientBase::connectionEstablished();
      }

      bool isIdle(void) const
      {
        return client->isIdle();
      }

      std::string remoteHostName(void) const
      {
        return client->remoteHostName();
      }

      const uint16_t remotePort(void)
      {
        return client->conObj()->remotePort();
      }
    };

      // Forward declared states
    struct StateTop;
    struct StateDisconnected;
    struct StateConnecting;
    struct StateConnectingSRVLookup;
    struct StateConnectingTryConnect;
    struct StateConnectingIdle;
    struct StateConnected;
    struct StateConnectedHighestPrio;
    struct StateConnectedLowerPrio;
    struct StateConnectedLowerPrioIdle;
    struct StateConnectedLowerPrioSRVLookup;
    struct StateConnectedLowerPrioTryConnect;

    struct StateTop
      : Async::StateTopBase<Context, StateTop>::Type
    {
      static constexpr auto NAME = "Top";

      void init(void) noexcept
      {
        setState<StateDisconnected>();
      }
      virtual void connectEvent(void) noexcept {}
      virtual void disconnectEvent(void) noexcept
      {
        DEBUG_EVENT;
        setState<StateDisconnected>();
      }
      virtual void dnsResultsReadyEvent(void) noexcept {}
      virtual void connectedEvent(void) noexcept {}
      virtual void disconnectedEvent(void) noexcept {}
      virtual void bgConnectedEvent(void) noexcept {}
      virtual void bgDisconnectedEvent(void) noexcept {}
    }; /* StateTop */


    struct StateDisconnected
      : Async::StateBase<StateTop, StateDisconnected>
    {
      static constexpr auto NAME = "Disconnected";

      void entry(void) noexcept
      {
        ctx().closeConnection();
      }

      virtual void connectEvent(void) noexcept override
      {
        DEBUG_EVENT;
        setState<StateConnectingSRVLookup>();
      }
    }; /* StateDisconnected */


    struct StateConnecting
      : Async::StateBase<StateTop, StateConnecting>
    {
      static constexpr auto NAME = "Connecting";
      void entry(void) noexcept
      {
        //ctx().connect_retry_wait.reset();
      }
    }; /* StateConnecting */


    struct StateConnectingSRVLookup
      : Async::StateBase<StateConnecting, StateConnectingSRVLookup>
    {
      static constexpr auto NAME = "ConnectingSRVLookup";

      void entry(void) noexcept
      {
        ctx().dns.lookup();
      }

      void exit(void) noexcept
      {
        ctx().dns.abort();
      }

      virtual void dnsResultsReadyEvent(void) noexcept override
      {
        DEBUG_EVENT;
        ctx().dns.resourceRecords(ctx().rrs);
#ifdef ASYNC_STATE_MACHINE_DEBUG
        std::cout << "### Found " << ctx().rrs.size() << " records"
                  << std::endl;
        for (auto it = ctx().rrs.begin(); it != ctx().rrs.end(); ++it)
        {
          std::cout << "### " << (*it)->toString() << std::endl;
        }
#endif
        if (!ctx().rrs.empty())
        {
          ctx().next_rr = ctx().rrs.end();
          setState<StateConnectingTryConnect>();
        }
        else
        {
          setState<StateConnectingIdle>();
        }
      }
    }; /* StateConnectingSRVLookup */


    struct StateConnectingTryConnect
      : Async::StateBase<StateConnecting, StateConnectingTryConnect>
    {
      static constexpr auto NAME = "ConnectingTryConnect";

      void entry(void) noexcept
      {
        connectToNext();
      }

      virtual void connectedEvent(void) noexcept override
      {
        DEBUG_EVENT;
        setState<StateConnected>();
      }

      virtual void disconnectedEvent(void) noexcept override
      {
        DEBUG_EVENT;
        connectToNext();
      }

      private:
        void connectToNext(void)
        {
          auto& next_rr = ctx().next_rr;
          if (next_rr == ctx().rrs.end())
          {
            next_rr = ctx().rrs.begin();
          }
          else if (!ctx().marked_as_established)
          {
            next_rr = std::next(next_rr);
          }
          if (next_rr == ctx().rrs.end())
          {
            setState<StateConnectingIdle>();
            return;
          }
#ifdef ASYNC_STATE_MACHINE_DEBUG
          std::cout << "### Connecting to " << (*next_rr)->target()
                    << ":" << (*next_rr)->port() << std::endl;
#endif
          ctx().marked_as_established = false;
          ctx().connect((*next_rr)->target(), (*next_rr)->port());
        }
    }; /* StateConnectingTryConnect */


    struct StateConnectingIdle
      : Async::StateBase<StateConnecting, StateConnectingIdle>
    {
      static constexpr auto NAME = "ConnectingIdle";

      void entry(void) noexcept
      {
        if (ctx().marked_as_established)
        {
          ctx().connect_retry_wait.reset();
        }
        setTimeout(ctx().connect_retry_wait);
      }

      void exit(void) noexcept
      {
        clearTimeout();
      }

      virtual void timeoutEvent(void) noexcept override
      {
        DEBUG_EVENT;
        setState<StateConnectingSRVLookup>();
      }
    }; /* StateConnectingIdle */


    struct StateConnected
      : Async::StateBase<StateTop, StateConnected>
    {
      static constexpr auto NAME = "Connected";

      void init(void) noexcept
      {
        if ((ctx().next_rr == ctx().rrs.begin()) &&
            !ctx().dns.lookupFailed())
        {
          setState<StateConnectedHighestPrio>();
        }
        else
        {
          setState<StateConnectedLowerPrioIdle>();
        }
      }

      void entry(void) noexcept
      {
        Application::app().runTask([&]{ ctx().emitConnected(); });
      }

      virtual void disconnectedEvent(void) noexcept override
      {
        DEBUG_EVENT;
        if (ctx().marked_as_established)
        {
          setState<StateConnectingIdle>();
        }
        else
        {
          setState<StateConnectingTryConnect>();
        }
      }
    }; /* StateConnected */


    struct StateConnectedHighestPrio
      : Async::StateBase<StateConnected, StateConnectedHighestPrio>
    {
      static constexpr auto NAME = "ConnectedHighestPrio";
    }; /* StateConnectedHighestPrio */


    struct StateConnectedLowerPrio
      : Async::StateBase<StateConnected, StateConnectedLowerPrio>
    {
      static constexpr auto NAME = "ConnectedLowerPrio";
    }; /* StateConnectedLowerPrio */


    struct StateConnectedLowerPrioIdle
      : Async::StateBase<StateConnectedLowerPrio, StateConnectedLowerPrioIdle>
    {
      static constexpr auto NAME = "ConnectedLowerPrioIdle";

      void entry(void) noexcept
      {
        struct timeval tv;
        auto err = gettimeofday(&tv, NULL);
        assert(err == 0);
        struct tm tm;
        time_t timeout_at = tv.tv_sec + 60;
        auto tm_ret = localtime_r(&timeout_at, &tm);
        assert(tm_ret == &tm);
        tm.tm_sec = 0;
        setTimeoutAt(tm, std::rand() % 500);
      }

      void exit(void) noexcept
      {
        clearTimeoutAt();
      }

      virtual void timeoutAtEvent(void) noexcept override
      {
        DEBUG_EVENT;
        setState<StateConnectedLowerPrioSRVLookup>();
      }
    }; /* StateConnectedLowerPrioIdle */


    struct StateConnectedLowerPrioSRVLookup
      : Async::StateBase<StateConnectedLowerPrio,
                         StateConnectedLowerPrioSRVLookup>
    {
      static constexpr auto NAME = "ConnectedLowerPrioSRVLookup";

      void entry(void) noexcept
      {
        ctx().dns.lookup();
      }

      void exit(void) noexcept
      {
        ctx().dns.abort();
      }

      virtual void dnsResultsReadyEvent(void) noexcept override
      {
        DEBUG_EVENT;
        ctx().dns.resourceRecords(ctx().rrs);
#ifdef ASYNC_STATE_MACHINE_DEBUG
        std::cout << "### Found " << ctx().rrs.size() << " records"
                  << std::endl;
        for (auto it = ctx().rrs.begin(); it != ctx().rrs.end(); ++it)
        {
          std::cout << "### " << (*it)->toString() << std::endl;
        }
#endif
        if (!ctx().rrs.empty() &&
            ((ctx().rrs.front()->target() != ctx().remoteHostName()) ||
             (ctx().rrs.front()->port() != ctx().remotePort())))
        {
          setState<StateConnectedLowerPrioTryConnect>();
        }
        else
        {
          setState<StateConnected>();
        }
      }
    }; /* StateConnectedLowerPrioSRVLookup */


    struct StateConnectedLowerPrioTryConnect
      : Async::StateBase<StateConnectedLowerPrio,
                         StateConnectedLowerPrioTryConnect>
    {
      static constexpr auto NAME = "ConnectedLowerPrioTryConnect";

      void entry(void) noexcept
      {
        ctx().next_rr = ctx().rrs.begin();
#ifdef ASYNC_STATE_MACHINE_DEBUG
        std::cout << "### Connecting to " << (*ctx().next_rr)->target()
                  << ":" << (*ctx().next_rr)->port() << std::endl;
#endif
        ctx().bg_con->conObj()->setRecvBufLen(
            ctx().client->conObj()->recvBufLen());
        ctx().bg_con->connect((*ctx().next_rr)->target(),
                              (*ctx().next_rr)->port());
      }

      void exit(void) noexcept
      {
        ctx().bg_con->disconnect();
      }

      virtual void bgConnectedEvent(void) noexcept override
      {
        DEBUG_EVENT;
        if (!ctx().isIdle())
        {
          ctx().closeConnection();
          ctx().emitDisconnected(TcpConnection::DR_SWITCH_PEER);
        }
        auto ssl_ctx = ctx().client->conObj()->sslContext();
        *reinterpret_cast<TcpClientBase*>(ctx().client) =
            std::move(*ctx().bg_con);
        ctx().client->conObj()->setSslContext(*ssl_ctx, false);
        Application::app().runTask(sigc::bind(
              [](Context& ctx)
              {
                ctx.emitConnected();
              },
              std::ref(ctx())));
        setState<StateConnected>();
      }

      virtual void bgDisconnectedEvent(void) noexcept override
      {
        DEBUG_EVENT;
        const auto& rr = ++ctx().next_rr;
        if ((rr != ctx().rrs.end()) &&
            (((*rr)->target() != ctx().remoteHostName()) ||
            ((*rr)->port() != ctx().remotePort())))
        {
#ifdef ASYNC_STATE_MACHINE_DEBUG
          std::cout << "### Connecting to " << (*ctx().next_rr)->target()
                    << ":" << (*ctx().next_rr)->port() << std::endl;
#endif
          ctx().bg_con->connect((*ctx().next_rr)->target(), (*ctx().next_rr)->port());
        }
        else
        {
          setState<StateConnectedLowerPrioIdle>();
        }
      }
    }; /* StateConnectedLowerPrioTryConnect */

    Context                                 ctx;
    Async::StateMachine<Context, StateTop>  m;

}; /* TcpPrioClientBase::Machine */


namespace {

/****************************************************************************
 *
 * Local functions
 *
 ****************************************************************************/


}; /* End of anonymous namespace */

/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

TcpPrioClientBase::TcpPrioClientBase(TcpConnection *con)
  : TcpClientBase(con)
{
} /* TcpPrioClientBase::TcpPrioClientBase */


TcpPrioClientBase::~TcpPrioClientBase(void)
{
  delete m_machine;
  m_machine = nullptr;
} /* TcpPrioClientBase::~TcpPrioClientBase */


void TcpPrioClientBase::setReconnectMinTime(unsigned t)
{
  m_machine->setReconnectMinTime(t);
} /* TcpPrioClientBase::setReconnectMinTime */


void TcpPrioClientBase::setReconnectMaxTime(unsigned t)
{
  m_machine->setReconnectMaxTime(t);
}


void TcpPrioClientBase::setReconnectBackoffPercent(unsigned p)
{
  m_machine->setReconnectBackoffPercent(p);
}


void TcpPrioClientBase::setReconnectRandomizePercent(unsigned p)
{
  m_machine->setReconnectRandomizePercent(p);
}


void TcpPrioClientBase::setService(const std::string& srv_name,
                                   const std::string& srv_proto,
                                   const std::string& srv_domain)
{
  assert(!srv_name.empty() && !srv_proto.empty() && !srv_domain.empty());
  std::string srv = "_";
  srv += srv_name;
  srv += "._";
  srv += srv_proto;
  srv += ".";
  srv += srv_domain;
  m_machine->setLookupParams(srv, DnsLookup::Type::SRV);
} /* TcpPrioClientBase::setService */


void TcpPrioClientBase::addStaticSRVRecord(
                        DnsResourceRecordSRV::Ttl     ttl,
                        DnsResourceRecordSRV::Prio    prio,
                        DnsResourceRecordSRV::Weight  weight,
                        DnsResourceRecordSRV::Port    port,
                        DnsResourceRecordSRV::Target  target)
{
  const auto& srv = m_machine->label();
  m_machine->addStaticResourceRecord(new DnsResourceRecordSRV(
        srv.empty() ? "static" : srv, ttl, prio, weight, port, target));
} /* TcpPrioClientBase::addStaticSRVRecord */


const std::string& TcpPrioClientBase::service(void) const
{
  return m_machine->label();
} /* TcpPrioClientBase::service */


void TcpPrioClientBase::connect(void)
{
  //m_successful_connect = false;
  m_machine->connect();
} /* TcpPrioClientBase::connect */


void TcpPrioClientBase::disconnect(void)
{
  m_machine->disconnect();
} /* TcpPrioClientBase::disconnect */


void TcpPrioClientBase::markAsEstablished(void)
{
  //std::cout << "### TcpPrioClientBase::markAsEstablished" << std::endl;
  m_machine->markAsEstablished();
} /* TcpPrioClientBase::markAsEstablished */


bool TcpPrioClientBase::markedAsEstablished(void) const
{
  return m_machine->markedAsEstablished();
} /* TcpPrioClientBase::markedAsEstablished */


bool TcpPrioClientBase::isIdle(void) const
{
  return m_machine->isIdle();
} /* TcpPrioClientBase::isIdle */


bool TcpPrioClientBase::isPrimary(void) const
{
  return m_machine->isPrimary();
} /* TcpPrioClientBase::isPrimary */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void TcpPrioClientBase::initialize(void)
{
  m_machine = new Machine(this);
} /* TcpPrioClientBase::initialize */


void TcpPrioClientBase::connectionEstablished(void)
{
  //m_successful_connect = true;
  //TcpClientBase::connectionEstablished();
  m_machine->connectionEstablished();
} /* TcpPrioClientBase::connectionEstablished */


void TcpPrioClientBase::onDisconnected(TcpConnection::DisconnectReason reason)
{
  m_machine->onDisconnected();
  //m_successful_connect = false;
} /* TcpPrioClientBase::onDisconnected */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 * This file has not been truncated
 */
