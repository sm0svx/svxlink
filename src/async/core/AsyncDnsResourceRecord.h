/**
@file   AsyncDnsResourceRecord.h
@brief  A collection of classes representing DNS resource records
@author Tobias Blomberg / SM0SVX
@date   2021-05-22

\verbatim
Async - A library for programming event driven applications
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

#ifndef ASYNC_DNS_RESOURCE_RECORD_INCLUDED
#define ASYNC_DNS_RESOURCE_RECORD_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <sstream>
#include <map>
#include <memory>
#include <vector>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncIpAddress.h>


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
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/



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
@brief  The base class for representing a DNS resource record
@author Tobias Blomberg / SM0SVX
@date   2021-05-22

This is the base class for representing a DNS resource record. One or more
resource records is the result of performing a DNS query.

\include AsyncDnsLookup_demo.cpp
*/
class DnsResourceRecord
{
  public:
    using Name = std::string;
    using Ttl = uint32_t;

    /**
     * @brief   The record type
     */
    enum class Type { ANY, A, PTR, CNAME, SRV };

    /**
     * @brief   The record class
     */
    enum class Class { IN };

    /**
     * @brief   The type for a list of resource records
     */
    using List = std::vector<std::unique_ptr<DnsResourceRecord>>;

    /**
     * @brief   The maximum allowed value for a TTL
     */
    static constexpr Ttl MAX_TTL = 0x7fffffff;

    /**
     * @brief   The type for this specific class
     */
    static const Type staticType(void) { return Type::ANY; }

    /**
     * @brief   The type for this specific class represented as a string
     */
    static const std::string& typeToString(Type type)
    {
      static const std::map<Type, std::string> type_map = {
        {Type::A, "A"}, {Type::PTR, "PTR"}, {Type::CNAME, "CNAME"},
        {Type::SRV, "SRV"}
      };
      static const std::string UNKNOWN = "?";
      const auto& it = type_map.find(type);
      if (it == type_map.end())
      {
        return UNKNOWN;
      }
      return it->second;
    }

    /**
     * @brief   Constructor
     * @param   name  The name of this record
     * @param   ttl   The time-to-live, in seconds, for this record
     */
    DnsResourceRecord(const Name& name, Ttl ttl)
      : m_name(name), m_ttl(ttl) {}

    /**
     * @brief   Destructor
     */
    virtual ~DnsResourceRecord(void) {}

    /**
     * @brief   Clone this class
     * @return  Return a pointer to a clone of this instance
     */
    virtual DnsResourceRecord* clone(void) const = 0;

    /**
     * @brief   Equality comparison operator
     * @param   other The other resource record to comapare to
     * @return  Return \em true if the two records are equal
     *
     * NOTE: The TTL is not used in the comparison.
     */
    virtual bool operator==(const DnsResourceRecord& other) const
    {
      return (classId() == other.classId()) &&
             (type() == other.type()) &&
             (name() == other.name());
    }

    /**
     * @brief   The DNS class for the record
     * @return  Return the DNS class for the record
     */
    Class classId(void) const { return Class::IN; }

    /**
     * @brief   The DNS class for the record as a string
     * @return  Return the DNS class as a string for this record
     */
    const char* classStr(void) const { return "IN"; }

    /**
     * @brief   The type of record
     * @return  Return the type of this record
     */
    virtual const Type type(void) const = 0;

    /**
     * @brief   The type of record as a string
     * @return  Return the type of this record as a string
     */
    const std::string& typeStr(void) const
    {
      return typeToString(type());
    }

    /**
     * @brief   The string representation of this record
     * @return  Return the string representation of this record
     */
    virtual std::string toString(void) const
    {
      std::ostringstream os;
      os << name() << "\t" << ttl() << "\t" << classStr() << "\t"
         << typeStr() << "\t";
      return os.str();
    }

    /**
     * @brief   Set the name for this record
     * @param   name the new name of the record
     */
    void setName(const Name& name) { m_name = name; }

    /**
     * @brief   The name of this record
     * @return  Return the name of this record
     */
    const Name& name(void) const { return m_name; }

    /**
     * @brief   Set the TTL for this record
     * @param   ttl The time-to-live, in seconds, for this record
     */
    void setTtl(Ttl ttl) { m_ttl = ttl; }

    /**
     * @brief   The TTL for this record
     * @return  Return the time-to-live, in seconds, for this record
     */
    Ttl ttl(void) const { return m_ttl; }

  private:
    Name  m_name;
    Ttl   m_ttl;

};  /* class DnsResourceRecord */


/**
@brief  A CRTP class for adding some type dependent functions
@author Tobias Blomberg / SM0SVX
@date   2021-05-22

All specific resource record classes inherit from this class to add some record
type specific functions and types to each specific resource record class.
*/
template <typename Derived>
class DnsResourceRecordCRTP : public DnsResourceRecord
{
  public:
    /**
     * @brief   The type for a list of resource records
     */
    using List = std::vector<std::unique_ptr<Derived>>;

      /**
       * @brief   Constructor
       * @param   name  The name of this record
       * @param   ttl   The time-to-live, in seconds, for this record
       */
    DnsResourceRecordCRTP(const Name& name, Ttl ttl)
      : DnsResourceRecord(name, ttl) {}

    /**
     * @brief   Clone this class
     * @return  Return a pointer to a clone of this instance
     */
    virtual DnsResourceRecord* clone(void) const
    {
      return new Derived(static_cast<Derived const&>(*this));
    }

    /**
     * @brief   Equality comparison operator
     * @param   other The other resource record to comapare to
     * @return  Return \em true if the two records are equal
     *
     * NOTE: The TTL is not used in the comparison.
     */
    virtual bool operator==(const DnsResourceRecord& other) const
    {
      const auto& lhs = static_cast<const Derived&>(*this);
      const auto& rhs = static_cast<const Derived&>(other);
      return lhs == rhs;
    }

    virtual bool operator==(const Derived& other) const = 0;

    /**
     * @brief   The type of record
     * @return  Return the type of this record
     */
    virtual const Type type(void) const override { return Derived::staticType(); }
}; /* DnsResourceRecordCRTP */


/**
@brief  A class for representing an A DNS resource record
@author Tobias Blomberg / SM0SVX
@date   2021-05-22

This class represents an A DNS resource record. One or more resource records is
the result of performing a DNS query. This specific resource record maps a
hostname to an IP address.

\include AsyncDnsLookup_demo.cpp
*/
class DnsResourceRecordA : public DnsResourceRecordCRTP<DnsResourceRecordA>
{
  public:
    using Ip = Async::IpAddress;

    /**
     * @brief   The type for this specific class
     */
    static const Type staticType(void) { return Type::A; }

    /**
     * @brief   Constructor
     * @param   name  The name of this record
     * @param   ttl   The time-to-live, in seconds, for this record
     * @param   ip    The IP address associated with the record name
     */
    DnsResourceRecordA(const Name& name, Ttl ttl, const Ip& ip=Ip())
      : DnsResourceRecordCRTP<DnsResourceRecordA>(name, ttl), m_ip(ip) {}

    /**
     * @brief   Equality comparison operator
     * @param   other The other resource record to comapare to
     * @return  Return \em true if the two records are equal
     *
     * NOTE: The TTL is not used in the comparison.
     */
    virtual bool operator==(const DnsResourceRecordA& other) const
    {
      return DnsResourceRecord::operator==(other) &&
             (ip() == other.ip());
    }

    /**
     * @brief   The string representation of this record
     * @return  Return the string representation of this record
     */
    virtual std::string toString(void) const
    {
      std::ostringstream os;
      os << DnsResourceRecord::toString() << ip();
      return os.str();
    }

    /**
     * @brief   Set the IP address for this record
     * @param   ip The new IP address to set
     */
    void setIp(const Ip& ip) { m_ip = ip; }

    /**
     * @brief   The IP address for this record
     * @return  Return the IP address for this record
     */
    const Ip& ip(void) const { return m_ip; }

  private:
    Ip m_ip;
}; /* DnsResourceRecordA */


/**
@brief  A class for representing a PTR DNS resource record
@author Tobias Blomberg / SM0SVX
@date   2021-05-22

This class represents a PTR DNS resource record. One or more resource records
is the result of performing a DNS query. This specific resource record maps an
IP address to hostnames.

\include AsyncDnsLookup_demo.cpp
*/
class DnsResourceRecordPTR : public DnsResourceRecordCRTP<DnsResourceRecordPTR>
{
  public:
    using DName = std::string;

    /**
     * @brief   The type for this specific class
     */
    static const Type staticType(void) { return Type::PTR; }

    /**
     * @brief   Constructor
     * @param   name  The name of this record
     * @param   ttl   The time-to-live, in seconds, for this record
     * @param   dname The FQDN associaated with this record name
     */
    DnsResourceRecordPTR(const Name& name, Ttl ttl, const DName& dname="")
      : DnsResourceRecordCRTP<DnsResourceRecordPTR>(name, ttl), m_dname(dname)
    {}

    /**
     * @brief   Equality comparison operator
     * @param   other The other resource record to comapare to
     * @return  Return \em true if the two records are equal
     *
     * NOTE: The TTL is not used in the comparison.
     */
    virtual bool operator==(const DnsResourceRecordPTR& other) const
    {
      return DnsResourceRecord::operator==(other) &&
             (dname() == other.dname());
    }

    /**
     * @brief   The string representation of this record
     * @return  Return the string representation of this record
     */
    virtual std::string toString(void) const
    {
      std::ostringstream os;
      os << DnsResourceRecord::toString() << dname();
      return os.str();
    }

    /**
     * @brief   Set the FQDN for this record
     * @param   dname The new FQDN for this record
     */
    void setName(const DName& dname) { m_dname = dname; }

    /**
     * @brief   The FQDN for this record
     * @return  Return the FQDN for this record
     */
    const DName& dname(void) const { return m_dname; }

  private:
    DName m_dname;
}; /* DnsResourceRecordPTR */


/**
@brief  A class for representing a CNAME DNS resource record
@author Tobias Blomberg / SM0SVX
@date   2021-05-22

This class represents a CNAME DNS resource record. One or more resource records
is the result of performing a DNS query. This specific resource record maps a
hostname to other hostnames.

\include AsyncDnsLookup_demo.cpp
*/
class DnsResourceRecordCNAME :
  public DnsResourceRecordCRTP<DnsResourceRecordCNAME>
{
  public:
    using CName = std::string;

    /**
     * @brief   The type for this specific class
     */
    static const Type staticType(void) { return Type::CNAME; }

    /**
     * @brief   Constructor
     * @param   name  The name of this record
     * @param   ttl   The time-to-live, in seconds, for this record
     * @param   cname The FQDN associaated with this record name
     */
    DnsResourceRecordCNAME(const Name& name, Ttl ttl, const CName& cname="")
      : DnsResourceRecordCRTP<DnsResourceRecordCNAME>(name, ttl),
        m_cname(cname) {}

    /**
     * @brief   Equality comparison operator
     * @param   other The other resource record to comapare to
     * @return  Return \em true if the two records are equal
     *
     * NOTE: The TTL is not used in the comparison.
     */
    virtual bool operator==(const DnsResourceRecordCNAME& other) const
    {
      return DnsResourceRecord::operator==(other) &&
             (cname() == other.cname());
    }

    /**
     * @brief   The string representation of this record
     * @return  Return the string representation of this record
     */
    virtual std::string toString(void) const
    {
      std::ostringstream os;
      os << DnsResourceRecord::toString() << cname();
      return os.str();
    }

    /**
     * @brief   Set the FQDN for this record
     * @param   cname The new FQDN for this record
     */
    void setName(const CName& cname) { m_cname = cname; }

    /**
     * @brief   The FQDN for this record
     * @return  Return the FQDN for this record
     */
    const CName& cname(void) const { return m_cname; }

  private:
    CName m_cname;
}; /* DnsResourceRecordCNAME */


/**
@brief  A class for representing a SRV DNS resource record
@author Tobias Blomberg / SM0SVX
@date   2021-05-22

This class represents an SRV DNS resource record. One or more resource records
is the result of performing a DNS query. This specific resource record maps a
service name to information for that service.

This is one of the more advanced resource record types. A query for this type
of resource record often return multiple entries. Each entry contain
information about priority, weight, port and target.

* Priority: The priority of the target host, lower value means more preferred.
* Weight:   A relative weight for records with the same priority, higher value
            means higher chance of getting picked.
* Port:     the TCP or UDP port on which the service is to be found.
* Target:   the canonical hostname of the machine providing the service, ending
            in a dot.

An example of SRV records in textual form that might be found in a zone file
might be the following:

  _sip._tcp.example.com. 86400 IN SRV 0 60  5060 sipserver1.example.com.
  _sip._tcp.example.com. 86400 IN SRV 0 40  5060 sipserver2.example.com.
  _sip._tcp.example.com. 86400 IN SRV 1 100 5061 sipserver-backup.example.com.

The client should choose sipserver1 or sipserver2 first. Sipserver1 should be
chosen 60% of the time and sipserver2 should be chosen 40% of the time. If none
of the two first servers are reachable the client should try to connect to
sipserver-backup.

\include AsyncDnsLookup_demo.cpp
*/
class DnsResourceRecordSRV :
  public DnsResourceRecordCRTP<DnsResourceRecordSRV>
{
  public:
    using Prio    = unsigned int;
    using Weight  = unsigned int;
    using Port    = unsigned int;
    using Target  = std::string;

    /**
     * @brief   The type for this specific class
     */
    static const Type staticType(void) { return Type::SRV; }

    /**
     * @brief   Constructor
     * @param   name    The name of this record
     * @param   ttl     The time-to-live, in seconds, for this record
     * @param   prio    The priority for this record
     * @param   weight  The weight for this record
     * @param   port    The network port for this record
     * @param   target  The FQDN associaated with this record name
     */
    DnsResourceRecordSRV(const Name& name, Ttl ttl, Prio prio, Weight weight,
                         Port port, const Target& target)
      : DnsResourceRecordCRTP<DnsResourceRecordSRV>(name, ttl),
        m_prio(prio), m_weight(weight), m_port(port), m_target(target) {}

    /**
     * @brief   Equality comparison operator
     * @param   other The other resource record to comapare to
     * @return  Return \em true if the two records are equal
     *
     * NOTE: The TTL is not used in the comparison.
     */
    virtual bool operator==(const DnsResourceRecordSRV& other) const
    {
      return DnsResourceRecord::operator==(other) &&
             (prio() == other.prio()) &&
             (weight() == other.weight()) &&
             (port() == other.port()) &&
             (target() == other.target());
    }

    /**
     * @brief   The string representation of this record
     * @return  Return the string representation of this record
     */
    virtual std::string toString(void) const
    {
      std::ostringstream os;
      os << DnsResourceRecord::toString() << prio() << " " << weight()
         << " " << port() << " " << target();
      return os.str();
    }

    /**
     * @brief   Set the prio for this record
     * @param   prio The new priority for this record (lower mean higher prio)
     */
    void setPrio(Prio prio) { m_prio = prio; }

    /**
     * @brief   The prio for this record
     * @return  The prio number for this record (lower mean higher prio)
     */
    Prio prio(void) const { return m_prio; }

    /**
     * @brief   Set the weight for this record
     * @param   weight The new weight for this record
     */
    void setWeight(Weight weight) { m_weight = weight; }

    /**
     * @brief   The weight for this record
     * @return  Return the weight for this record
     */
    Weight weight(void) const { return m_weight; }

    /**
     * @brief   Set the network port for this record
     * @param   port The new network port number
     */
    void setPort(Port port) { m_port = port; }

    /**
     * @brief   The network port for this record
     * @return  Return the network port number
     */
    Port port(void) const { return m_port; }

    /**
     * @brief   Set the FQDN for this record
     * @param   target The new FQDN for this record
     */
    void setTarget(const Target& target) { m_target = target; }

    /**
     * @brief   The FQDN for this record
     * @return  Return the FQDN for this record
     */
    const Target& target(void) const { return m_target; }

  private:
    Prio    m_prio;
    Weight  m_weight;
    Port    m_port;
    Target  m_target;
}; /* DnsResourceRecordSRV */


} /* namespace Async */

#endif /* ASYNC_DNS_RESOURCE_RECORD_INCLUDED */

/*
 * This file has not been truncated
 */
