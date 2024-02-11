/**
@file	 AsyncDnsLookupWorker.h
@brief   Contains a base class for implementing a DNS lookup worker object
@author  Tobias Blomberg
@date	 2003-04-12

This file contains a class that is used as a base class for a DNS lookup
worker. That is an object that do the actual work when a DNS query have
been created using a Async::DnsQuery class. This worker class is only
used internally by the async library.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2024 Tobias Blomberg

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



#ifndef ASYNC_DNS_LOOKUP_WORKER_INCLUDED
#define ASYNC_DNS_LOOKUP_WORKER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdlib.h>
#include <time.h>
#include <sigc++/sigc++.h>

#include <vector>
#include <set>
#include <cassert>
#include <algorithm>
#include <chrono>
#include <limits>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncIpAddress.h>
#include <AsyncDnsResourceRecord.h>
#include <AsyncApplication.h>
#include <AsyncDnsLookup.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

namespace Async
{

/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief	DNS lookup worker base class
@author Tobias Blomberg
@date   2003-04-12

This is the DNS lookup worker base class. It is an internal class that should
only be used from within the async library.
*/
class DnsLookupWorker
{
  public:
    /**
     * @brief 	Constructor
     */
    DnsLookupWorker(const DnsLookup& dns)
      : m_dns(dns)
    {
      //std::cout << "### DnsLookupWorker::DnsLookupWorker: m_lookup_pending="
      //          << m_lookup_pending << std::endl;
      srand(time(0));
    }

    /**
     * @brief 	Destructor
     */
    virtual ~DnsLookupWorker(void)
    {
      clearResourceRecords(m_rr_tmp);
      clearResourceRecords(m_srv_records);
      m_srv_weight_sum.clear();
      m_lookup_pending = false;
      clearResourceRecords();
    }

    /**
     * @brief   Move assignment operator
     * @param   other The other object to move data from
     * @return  Returns this object
     */
    virtual DnsLookupWorker& operator=(DnsLookupWorker&& other)
    {
      //std::cout << "### DnsLookupWorker::operator=(&&)" << std::endl;

      m_rr_tmp.swap(other.m_rr_tmp);
      clearResourceRecords(other.m_rr_tmp);

      m_resource_records.swap(other.m_resource_records);
      clearResourceRecords(other.m_resource_records);

      m_srv_records.swap(other.m_srv_records);
      clearResourceRecords(other.m_srv_records);

      m_srv_weight_sum.swap(other.m_srv_weight_sum);
      other.m_srv_weight_sum.clear();

      m_answer_ts = std::move(other.m_answer_ts);

      m_min_ttl = std::move(other.m_min_ttl);

      m_lookup_pending = other.m_lookup_pending;
      other.m_lookup_pending = false;

      return *this;
    }

    /**
     * @brief   Called by the DnsLookup class to start the lookup
     * @return  Return \em true on success or else \em false
     */
    bool lookup(void)
    {
      //std::cout << "### DnsLookupWorker::lookup: m_lookup_pending="
      //          << m_lookup_pending << std::endl;

      if (m_lookup_pending)
      {
        return true;
      }

      if (recordsValid())
      {
          // Update TTL values for all records in the cache
        auto now = Clock::now();
        auto min_ttl = std::numeric_limits<DnsResourceRecord::Ttl>::max();
        for (auto& rr : m_resource_records)
        {
          auto age = std::chrono::duration_cast<std::chrono::seconds>(
              now - m_answer_ts).count();
          assert(age >= 0);
          DnsResourceRecord::Ttl new_ttl = 0;
          if (age < rr->ttl())
          {
            new_ttl = rr->ttl() - age;
          }
          rr->setTtl(new_ttl);
          min_ttl = std::min(min_ttl, new_ttl);
        }
        m_min_ttl = std::chrono::seconds(min_ttl);
        if (min_ttl > 0)
        {
          m_answer_ts = now;
        }
        m_min_ttl += std::chrono::seconds(10);

        resultsReady();
        return true;
      }

      if (dns().label().empty())
      {
        workerDone();
        return true;
      }

      m_lookup_pending = true;
      return doLookup();
    }

    /**
     * @brief   Check if a lookup is pending
     * @return  Return \em true if a lookup is pending
     */
    bool lookupPending(void) const { return m_lookup_pending; }

    /**
     * @brief   Check if the lookup failed
     * @return  Returns \em true if the lookup failed in any way
     *
     * Use this function to check if the DNS lookup completed without any
     * errors. Even if the lookup is marked as failed it may still contain
     * valid records.
     */
    bool lookupFailed(void) const { return m_lookup_failed; }

    /**
     * @brief   Abort a pending lookup
     */
    void abort(void)
    {
      abortLookup();
      clearResourceRecords(m_rr_tmp);
      clearResourceRecords(m_srv_records);
      m_srv_weight_sum.clear();
      m_lookup_pending = false;
    }

    /**
     * @brief   Remove all existing resource records
     */
    void clearResourceRecords(void)
    {
      clearResourceRecords(m_resource_records);
      //clearResourceRecords(m_rr_tmp);
    }

    /**
     * @brief   Check if the cached records are valid
     * @return  Return \em true if all records in the cache is valid
     *
     * This function is used to check if the cache contain resource records
     * that are still valid. A cached record become invalid when the
     * time-to-live, TTL, expires.
     */
    bool recordsValid(void) const
    {
      return !resourceRecords().empty() &&
             ((m_answer_ts + m_min_ttl) >= Clock::now());
    }

    /**
     * @brief   Return all resource records
     * @return  Returns all resource records that was found
     */
    const std::vector<DnsResourceRecord*>& resourceRecords(void) const
    {
      return m_resource_records;
    }

    /**
     * @brief 	A signal to indicate that the query has been completed
     */
    sigc::signal<void> resultsReady;

  protected:
    const DnsLookup& dns(void) { return m_dns; }

    /**
     * @brief   Called by the DnsLookupWorker class to start the lookup
     * @return  Return \em true on success or else \em false
     */
    virtual bool doLookup(void) = 0;

    /**
     * @brief   Called by the DnsLookupWorker class to abort a pending lookup
     * @return  Return \em true on success or else \em false
     */
    virtual void abortLookup(void) = 0;

    /**
     * @brief   Add a new resource record to the result list
     * @param   rr A new resource record
     *
     * The lookup worker use this function to add resource records to the
     * result list. The resource record will be owned by this class so must not
     * be deleted by the caller.
     */
    void addResourceRecord(DnsResourceRecord* rr)
    {
      if (rr->type() == DnsResourceRecordSRV::staticType())
      {
        auto srv_rr = dynamic_cast<DnsResourceRecordSRV*>(rr);
        assert(srv_rr != nullptr);
        m_srv_records.insert(srv_rr);
        m_srv_weight_sum[srv_rr->prio()] += srv_rr->weight();
      }
      else
      {
        m_rr_tmp.push_back(rr);
      }
    }

    /**
     * @brief   Called by the lookup worker when done
     */
    void workerDone(void)
    {
      m_lookup_pending = false;
      m_answer_ts = Clock::now();

      bool only_static = m_srv_records.empty();
      for (const auto& rr : dns().staticResourceRecords())
      {
        auto cloned_rr = rr->clone();
        if ((rr->ttl() == 0) && !only_static)
        {
          cloned_rr->setTtl(DnsResourceRecord::MAX_TTL);
        }
        addResourceRecord(cloned_rr);
      }

        // Order any SRV records in the order of preference.
        // Lowest priority number means highest priority.
        // Records with equal priority is ordered randomly by their relative
        // weight. A larger weight means a higher chance of getting picked.
      auto srv_it = m_srv_records.begin();
      while (srv_it != m_srv_records.end())
      {
        unsigned int weight_sum = m_srv_weight_sum[(*srv_it)->prio()];
        int weight_rand = 0;
        if (weight_sum > 0)
        {
          int rand_limit = RAND_MAX - (RAND_MAX % weight_sum);
          while ((weight_rand = rand()) >= rand_limit);
          weight_rand %= weight_sum;

          for (;;)
          {
            weight_rand -= (*srv_it)->weight();
            if (weight_rand < 0)
            {
              break;
            }
            ++srv_it;
          }
        }

        m_rr_tmp.push_back(*srv_it);
        m_srv_weight_sum[(*srv_it)->prio()] -= (*srv_it)->weight();
        m_srv_records.erase(srv_it);
        srv_it = m_srv_records.begin();
      }
      assert(m_srv_records.empty());
      m_srv_weight_sum.clear();

      auto min_ttl = std::numeric_limits<DnsResourceRecord::Ttl>::max();
      bool use_new_rrs = (m_resource_records.size() != m_rr_tmp.size());
      for (const auto& rr_tmp : m_rr_tmp)
      {
        auto it = std::find_if(
            m_resource_records.begin(),
            m_resource_records.end(),
            [&](DnsResourceRecord* rr) { return *rr == *rr_tmp; });
        if (it != m_resource_records.end())
        {
          (*it)->setTtl(rr_tmp->ttl());
        }
        else
        {
          use_new_rrs = true;
        }
        min_ttl = std::min(min_ttl, rr_tmp->ttl());
      }

      if (use_new_rrs)
      {
        m_resource_records.swap(m_rr_tmp);
      }
      clearResourceRecords(m_rr_tmp);

      m_min_ttl = std::chrono::seconds(min_ttl);

      resultsReady();
    }

    /**
     * @brief   Called by the lookup worker to mark the lookup as failed
     * @param   failed Set to \em false to clear the failure state
     */
    void setLookupFailed(bool failed=true) { m_lookup_failed = failed; }

  private:
    using Clock = std::chrono::steady_clock;

    struct CompSRV
    {
      bool operator()(const DnsResourceRecordSRV* lhs,
                      const DnsResourceRecordSRV* rhs)
      {
        return lhs->prio() < rhs->prio();
      }
    };

    const DnsLookup&                              m_dns;
    std::vector<DnsResourceRecord*>               m_rr_tmp;
    std::vector<DnsResourceRecord*>               m_resource_records;
    std::multiset<DnsResourceRecordSRV*, CompSRV> m_srv_records;
    std::map<unsigned int, unsigned int>          m_srv_weight_sum;
    Clock::time_point                             m_answer_ts;
    std::chrono::seconds                          m_min_ttl;
    bool                                          m_lookup_pending = false;
    bool                                          m_lookup_failed = false;

    template <class Container>
    void clearResourceRecords(Container& rrs)
    {
      for (auto& rr : rrs)
      {
        delete rr;
      }
      rrs.clear();
    }

};  /* class DnsLookupWorker */


} /* namespace */

#endif /* ASYNC_DNS_LOOKUP_WORKER_INCLUDED */



/*
 * This file has not been truncated
 */

