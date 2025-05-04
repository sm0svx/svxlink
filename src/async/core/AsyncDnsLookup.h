/**
@file	 AsyncDnsLookup.h
@brief   Contains a class for executing DNS queries
@author  Tobias Blomberg
@date	 2003-04-12

This file contains a class for executing DNS queries asynchronously. When
the answer arrives, a signal will be emitted. See the class documentation
for usage instructions.

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

/** @example  AsyncDnsLookup_demo.cpp
An example of how to use the Async::DnsLookup class
*/



#ifndef ASYNC_DNS_LOOKUP_INCLUDED
#define ASYNC_DNS_LOOKUP_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

#include <vector>
#include <memory>
#include <random>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncIpAddress.h>
#include <AsyncDnsResourceRecord.h>


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

class DnsLookupWorker;


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
@brief	A class for performing asynchronous DNS lookups
@author Tobias Blomberg
@date   2003-04-12

Use this class to make DNS lookups. Right now it supports looking up A, PTR,
CNAME and SRV records. An example usage can be seen below.

\include AsyncDnsLookup_demo.cpp
*/
class DnsLookup : public sigc::trackable
{
  public:
    using Type = DnsResourceRecord::Type;
    template <class RR> using RRList = std::vector<std::unique_ptr<RR>>;
    template <class RR> using SharedRRList = std::vector<std::shared_ptr<RR>>;

    /**
     * @brief   Default Constructor
     */
    DnsLookup(void);

    /**
     * @brief 	Constructor
     * @param 	label The label (e.g. hostname) to lookup
     * @param	type  What kind of resource record to look up
     */
    DnsLookup(const std::string& label, Type type=Type::A);

    /**
     * @brief 	Destructor
     */
    ~DnsLookup(void);

    /**
     * @brief   Move assignment operator
     * @param   other The other object to move data from
     * @return  Returns this object
     */
    DnsLookup& operator=(DnsLookup&& other);

    /**
     * @brief   Prepare a lookup by setting up query parameters
     * @param   label The label (e.g. hostname) to lookup
     * @param   type  What kind of resource record to look up
     * @return  Return \em true if the lookup process has been started
     *
     * Use this function to prepare a DNS lookup by setting up the query
     * parameters.  If the label or type does not match a currently pending
     * query, the pending query will be aborted.
     * To start a query, the lookup() function need to be called.
     */
    void setLookupParams(const std::string& label, Type type=Type::A);

    /**
     * @brief   Start a DNS lookup
     * @param   label The label (e.g. hostname) to lookup
     * @param   type  What kind of resource record to look up
     * @return  Return \em true if the lookup process has been started
     *
     * Use this function to start a DNS lookup. If a lookup is already running
     * for the given label/type the lookup request will be ignored. If the
     * label or type does not match a currently pending query, the pending
     * query will be aborted.
     * NOTE: This function may emit the resultsReady signal before returning.
     */
    bool lookup(const std::string& label, Type type=Type::A);

    /**
     * @brief   Start a DNS lookup using previously configured parameters
     * @return  Return \em true if the lookup process has been started
     *
     * Use this function to start a DNS lookup using parameters previously
     * configured. If a lookup is already running the request will be ignored.
     * NOTE: This function may emit the resultsReady signal before returning.
     */
    bool lookup(void);

    /**
     * @brief   Abort a pending lookup
     */
    void abort(void);

    /**
     * @brief   Return the type of lookup
     * @return  Returns the lookup type
     */
    Type type(void) const { return m_type; }

    /**
     * @brief   Return the type of lookup as a string
     * @return  Returns a string representation of the lookup type
     */
    std::string typeStr(void) const
    {
      return DnsResourceRecord::typeToString(type());
    }

    /**
     * @brief  Return the associated label
     * @return Returns the label associated with this DNS lookup
     */
    const std::string &label(void) const { return m_label; }

    /**
     * @brief   Check if a DNS lookup is pending
     * @return  Return \em true if a DNS lookup is pending
     */
    bool isPending(void) const;

    /**
     * @brief   Check if the lookup failed
     * @return  Returns \em true if the lookup failed in any way
     *
     * Use this function to check if the DNS lookup completed without any
     * errors. Even if the lookup is marked as failed it may still contain
     * valid records.
     */
    bool lookupFailed(void) const;

    /**
     * @brief  Check if the DNS lookup is done or not
     * @return Returns \em true if results are ready or \em false if not
     */
    bool resultsAreReady(void) const;

    /**
     * @brief   Check if the cached records are valid
     * @return  Return \em true if all records in the cache is valid
     *
     * This function is used to check if the cache contain resource records
     * that are still valid. A cached record become invalid when the
     * time-to-live, TTL, expires.
     */
    bool recordsValid(void) const;

    /**
     * @brief   Check if the query returned any answers
     * @return  Return \em true if there are any answer records
     */
    bool empty(void) const { return resourceRecordsP().empty(); }

    /**
     * @brief   Remove any cached DNS resource records
     */
    void clear(void);

    /**
     * @brief   Add a static resource record to the lookup result
     * @param   rr The resource record to add
     *
     * Use this function to add a static resource record to the DNS lookup
     * result. Static resource records will be added to the looked up resource
     * records before signalling that the results are ready, just as if they
     * have been received from the DNS system.
     * If the lookup label is empty when a lookup is initiated, no real lookup
     * will be performed. Only static resource records will be returned in that
     * case.
     */
    void addStaticResourceRecord(DnsResourceRecord* rr);

    /**
     * @brief   Get all previously added static resource records
     * @return  Returns a vector of resource records
     */
    const RRList<DnsResourceRecord>& staticResourceRecords(void) const
    {
      return m_static_rrs;
    }

    /**
     * @brief 	Return the addresses for the host in the query
     * @return	Return a stl vector which contains all the addresses
     *	      	associated with the hostname in the query.
     * @pre   	The result is not available before the resultsReay signal
     *	      	has been emitted
     *
     * Use this function to retrieve all the IP-addresses associated with
     * the hostname in the query. Use the lookupFailed() function to find out
     * if the query was successful or not.
     *
     * The order of the hosts in the returned vector will be randomized on each
     * call to this function.
     */
    std::vector<IpAddress> addresses(void);

    /**
     * @brief   Return all matching resource records
     * @return  Returns all matching resource records in an stl vector
     *
     * Use this function to return all resource records, that is of the given
     * type, that the query resulted in. If no type is given the function will
     * return all resource records.
     */
    DnsResourceRecord::List resourceRecords(Type type=Type::ANY) const
    {
      DnsResourceRecord::List rrs;
      cloneResourceRecords(rrs, type);
      return rrs;
    }

    /**
     * @brief   Return resource records of a specific type
     * @param   rrs Store RRs in this vector that match the RR type
     *
     * Use this template function to get all resource records of a specific
     * type. E.g.
     *
     *   std::vector<Async::DnsResourceRecordA> rrs;
     *   dns_lookup.resourceRecords(rrs);
     *
     * That code will return only the A records.
     */
    template <class RR>
    void resourceRecords(RRList<RR>& rrs) const
    {
      rrs.clear();
      cloneResourceRecords(rrs, RR::staticType());
    }

    template <class RR>
    void resourceRecords(SharedRRList<RR>& rrs) const
    {
      rrs.clear();
      cloneResourceRecords(rrs, RR::staticType());
    }

    /**
     * @brief 	A signal to indicate that the query has been completed
     * @param 	dns A reference to the DNS object associated with the query
     */
    sigc::signal<void, DnsLookup&> resultsReady;

  private:
    typedef std::vector<DnsResourceRecord*> RRListP;

    std::string                 m_label;
    Type                        m_type          = Type::A;
    DnsLookupWorker*            m_worker        = 0;
    RRList<DnsResourceRecord>   m_static_rrs;
    std::default_random_engine  m_rng;

    void onResultsReady(void);
    const RRListP& resourceRecordsP(void) const;

    template <class RR>
    void cloneResourceRecords(RRList<RR>& rrs, Type type) const
    {
      for (const auto& rr : resourceRecordsP())
      {
        if ((type == RR::Type::ANY) || (rr->type() == type))
        {
          rrs.push_back(std::unique_ptr<RR>(static_cast<RR*>(rr->clone())));
        }
      }
    }

    template <class RR>
    void cloneResourceRecords(SharedRRList<RR>& rrs, Type type) const
    {
      for (const auto& rr : resourceRecordsP())
      {
        if ((type == RR::Type::ANY) || (rr->type() == type))
        {
          rrs.push_back(std::shared_ptr<RR>(static_cast<RR*>(rr->clone())));
        }
      }
    }

};  /* class DnsLookup */


} /* namespace */

#endif /* ASYNC_DNS_LOOKUP_INCLUDED */



/*
 * This file has not been truncated
 */

