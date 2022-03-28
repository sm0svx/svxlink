/**
@file	 AsyncQtDnsLookupWorker.cpp
@brief   Execute DNS queries in the Qt environment
@author  Tobias Blomberg
@date	 2003-04-12

This file contains a class for executing DNS quries in the Qt variant of
the async environment. This class should never be used directly. It is
used by Async::QtApplication to execute DNS queries.

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

#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncDnsResourceRecord.h>
#include <AsyncDnsLookup.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncQtDnsLookupWorker.h"


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



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

QtDnsLookupWorker::QtDnsLookupWorker(const DnsLookup& dns)
  : DnsLookupWorker(dns)
{
} /* QtDnsLookupWorker::QtDnsLookupWorker */


QtDnsLookupWorker::~QtDnsLookupWorker(void)
{
  abortLookup();
} /* QtDnsLookupWorker::~QtDnsLookupWorker */


DnsLookupWorker& QtDnsLookupWorker::operator=(DnsLookupWorker&& other_base)
{
  this->DnsLookupWorker::operator=(std::move(other_base));

  auto& other = static_cast<QtDnsLookupWorker&>(other_base);

  other.abortLookup();
  m_lookup_id = -1;

  doLookup();

  return *this;
} /* QtDnsLookupWorker::operator=(DnsLookupWorker&&) */


bool QtDnsLookupWorker::doLookup(void)
{
  assert(dns().type() == DnsLookup::Type::A);
  m_lookup_id = QHostInfo::lookupHost(dns().label().c_str(), this,
                                      SLOT(onResultsReady(QHostInfo)));
  return true;
} /* QtDnsLookupWorker::doLookup */


void QtDnsLookupWorker::abortLookup(void)
{
  if (m_lookup_id != -1)
  {
    QHostInfo::abortHostLookup(m_lookup_id);
    m_lookup_id = -1;
  }
} /* QtDnsLookupWorker::abortLookup */


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

void QtDnsLookupWorker::onResultsReady(const QHostInfo &info)
{
  m_lookup_id = -1;

  if (info.error() != QHostInfo::NoError)
  {
    std::cerr << "*** ERROR: DNS lookup error: "
              << info.errorString().toStdString()
              << std::endl;
    workerDone();
    return;
  }

  QList<QHostAddress> list = info.addresses();
  QList<QHostAddress>::Iterator it = list.begin();
  while (it != list.end())
  {
    if ((*it).protocol() == QAbstractSocket::IPv4Protocol)
    {
      IpAddress ip((*it).toString().toStdString());
      addResourceRecord(new DnsResourceRecordA(dns().label(), 1, ip));
    }
    ++it;
  }

  workerDone();
} /* QtDnsLookupWorker::onResultsReady */


/*
 * This file has not been truncated
 */
