/**
@file	 AsyncDnsLookup.cpp
@brief   Contains a class for executing DNS queries
@author  Tobias Blomberg
@date	 2003-04-12

This file contains a class for executing DNS queries asynchronously. When
the answer arrives, a signal will be emitted. See the class documentation
for usage instructions.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2025  Tobias Blomberg

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

#include "AsyncApplication.h"
#include "AsyncDnsLookupWorker.h"
#include "AsyncDnsLookup.h"
#include "AsyncDnsResourceRecord.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace sigc;
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



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

DnsLookup::DnsLookup(void)
{
  m_worker = Application::app().newDnsLookupWorker(*this);
  m_worker->resultsReady.connect(
      sigc::mem_fun(*this, &DnsLookup::onResultsReady));
} /* DnsLookup::DnsLookup */


DnsLookup::DnsLookup(const string& label, DnsLookup::Type type)
  : DnsLookup()
{
  lookup(label, type);
} /* DnsLookup::DnsLookup */


DnsLookup::~DnsLookup(void)
{
  delete m_worker;
  m_worker = 0;
} /* DnsLookup::~DnsLookup */


DnsLookup& DnsLookup::operator=(DnsLookup&& other)
{
  //std::cout << "### DnsLookup::operator=(&&)" << std::endl;

  m_label = other.m_label;
  other.m_label.clear();

  m_type = other.m_type;
  other.m_type = Type::A;

  *m_worker = std::move(*other.m_worker);

  m_static_rrs = std::move(other.m_static_rrs);
  other.m_static_rrs.clear();

  return *this;
} /* DnsLookup::operator=(DnsLookup&&) */


void DnsLookup::setLookupParams(const std::string& label, Type type)
{
  if ((label != m_label) || (type != m_type))
  {
    abort();
    m_worker->clearResourceRecords();
    m_label = label;
    m_type = type;
  }
} /* DnsLookup::setLookupParams */


bool DnsLookup::lookup(const std::string& label, Type type)
{
  //std::cout << "### DnsLookup::lookup: label=" << label << std::endl;
  setLookupParams(label, type);
  return lookup();
} /* DnsLookup::lookup */


bool DnsLookup::lookup(void)
{
  m_worker->lookup();
  return true;
} /* DnsLookup::lookup */


void DnsLookup::abort(void)
{
  m_worker->abort();
} /* DnsLookup::abort */


bool DnsLookup::isPending(void) const
{
  return m_worker->lookupPending();
} /* DnsLookup::isPending */


bool DnsLookup::lookupFailed(void) const
{
  return m_worker->lookupFailed();
} /* DnsLookup::lookupFailed */


bool DnsLookup::resultsAreReady(void) const
{
  return !isPending() && !m_worker->resourceRecords().empty();
} /* DnsLookup::resultsAreReady */


bool DnsLookup::recordsValid(void) const
{
  return m_worker->recordsValid();
} /* DnsLookup::recordsValid */


void DnsLookup::clear(void)
{
  abort();
  m_worker->clearResourceRecords();
} /* DnsLookup::clear */


void DnsLookup::addStaticResourceRecord(DnsResourceRecord* rr)
{
  m_static_rrs.push_back(std::unique_ptr<DnsResourceRecord>(rr));
} /* DnsLookup::addStaticResourceRecord */


vector<IpAddress> DnsLookup::addresses(void)
{
  vector<IpAddress> addrs;
  for (const auto& rr : resourceRecordsP())
  {
    if (rr->type() == Type::A)
    {
      addrs.push_back(static_cast<const DnsResourceRecordA*>(rr)->ip());
    }
  }
  std::shuffle(addrs.begin(), addrs.end(), m_rng);
  return addrs;
} /* DnsLookup::addresses */


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

void DnsLookup::onResultsReady(void)
{
  resultsReady(*this);
} /* DnsLookup::onResultsReady */


const DnsLookup::RRListP& DnsLookup::resourceRecordsP(void) const
{
  return m_worker->resourceRecords();
} /* DnsLookup::resourceRecordsP */


/*
 * This file has not been truncated
 */
