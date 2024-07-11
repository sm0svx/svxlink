/**
@file   AsyncSslX509Extensions.h
@brief  A_brief_description_for_this_file
@author Tobias Blomberg / SM0SVX
@date   2022-05-22

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
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

/** @example AsyncSslX509Extensions_demo.cpp
An example of how to use the SslX509Extensions class
*/

#ifndef ASYNC_SSL_X509_EXTENSIONS_INCLUDED
#define ASYNC_SSL_X509_EXTENSIONS_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <openssl/x509v3.h>
#include <cstring>
#include <sstream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncIpAddress.h>
#include <AsyncSslX509ExtSubjectAltName.h>


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
@brief  A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2022-05-22

A_detailed_class_description

\include AsyncSslX509Extensions_demo.cpp
*/
class SslX509Extensions
{
  public:
    /**
     * @brief   Default constructor
     */
    SslX509Extensions(void)
    {
      m_exts = sk_X509_EXTENSION_new_null();
    }

    explicit SslX509Extensions(STACK_OF(X509_EXTENSION)* exts)
      : m_exts(exts)
    {
    }

    /**
     * @brief   Move Constructor
     * @param   other The object to move from
     */
    SslX509Extensions(SslX509Extensions&& other)
    {
      m_exts = other.m_exts;
      other.m_exts = nullptr;
    }

    /**
     * @brief   Copy constructor
     */
    explicit SslX509Extensions(const SslX509Extensions& other)
    {
      std::cout << "### SslX509Extensions copy constructor" << std::endl;
      for (int i=0; i<X509v3_get_ext_count(other.m_exts); ++i)
      {
        const auto ext = X509v3_get_ext(other.m_exts, i);
        X509v3_add_ext(&m_exts, ext, -1);
      }
    }

    /**
     * @brief   Disallow copy assignment
     */
    SslX509Extensions& operator=(const SslX509Extensions&) = delete;

    /**
     * @brief   Destructor
     */
    ~SslX509Extensions(void)
    {
      if (m_exts != nullptr)
      {
        sk_X509_EXTENSION_pop_free(m_exts, X509_EXTENSION_free);
        m_exts = nullptr;
      }
    }

    /**
     * @brief   A_brief_member_function_description
     * @param   param1 Description_of_param1
     * @return  Return_value_of_this_member_function
     */
    bool addBasicConstraints(const std::string& bc)
    {
      return addExt(NID_basic_constraints, bc);
    }

    bool addKeyUsage(const std::string& ku)
    {
      return addExt(NID_key_usage, ku);
    }

    bool addExtKeyUsage(const std::string& eku)
    {
      return addExt(NID_ext_key_usage, eku);
    }

    bool addSubjectAltNames(const std::string& san)
    {
      return addExt(NID_subject_alt_name, san);
    }

    SslX509ExtSubjectAltName subjectAltName(void) const
    {
      int ext_idx = X509v3_get_ext_by_NID(m_exts, NID_subject_alt_name, -1);
      if (ext_idx < 0)
      {
        return nullptr;
      }
      auto ext = X509v3_get_ext(m_exts, ext_idx);
      return ext;
    }

    bool addExtension(const SslX509ExtSubjectAltName& san)
    {
      const X509_EXTENSION* other_ext = san;
#if OPENSSL_VERSION_MAJOR >= 3
      auto ext = X509_EXTENSION_dup(other_ext);
#else
      auto ext = X509_EXTENSION_dup(const_cast<X509_EXTENSION*>(other_ext));
#endif
      return (sk_X509_EXTENSION_push(m_exts, ext) > 0);
    }

    operator const STACK_OF(X509_EXTENSION)*() const { return m_exts; }

  private:
    STACK_OF(X509_EXTENSION)* m_exts  = nullptr;

    bool addExt(int nid, const std::string& value)
    {
      auto ex = X509V3_EXT_conf_nid(NULL, NULL, nid, value.c_str());
      if (ex == nullptr)
      {
        return false;
      }
      sk_X509_EXTENSION_push(m_exts, ex);
      return true;
    }

    X509_EXTENSION* cloneExtension(int nid) const
    {
      int ext_idx = X509v3_get_ext_by_NID(m_exts, nid, -1);
      if (ext_idx < 0)
      {
        return nullptr;
      }
      auto ext = X509v3_get_ext(m_exts, ext_idx);
      return X509_EXTENSION_dup(ext);
    }

}; /* SslX509Extensions */



} /* namespace Async */

#endif /* ASYNC_SSL_X509_EXTENSIONS_INCLUDED */

/*
 * This file has not been truncated
 */
