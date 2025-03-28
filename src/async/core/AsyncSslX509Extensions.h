/**
@file   AsyncSslX509Extensions.h
@brief  A class representing X.509 v3 extensions
@author Tobias Blomberg / SM0SVX
@date   2022-05-22

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2025 Tobias Blomberg / SM0SVX

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

/** @example AsyncSslX509_demo.cpp
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
@brief  A class representing X.509 extensions
@author Tobias Blomberg / SM0SVX
@date   2022-05-22

\include AsyncSslX509_demo.cpp
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

    /**
     * @brief   Constructor
     * @param   exts Pointer to a stack of X509_EXTENSION objects
     */
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
     * @param   other The object to copy from
     */
    explicit SslX509Extensions(const SslX509Extensions& other)
    {
      //std::cout << "### SslX509Extensions copy constructor" << std::endl;
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
     * @brief   Add basic constraints extension
     * @param   bc Basic constraints string
     * @return  Return \em true on success
     *
     * Ex: addBasicConstraints("critical, CA:FALSE");
     */
    bool addBasicConstraints(const std::string& bc)
    {
      return addExt(NID_basic_constraints, bc);
    }

    /**
     * @brief   Add key usage
     * @param   ku Key usage string
     * @return  Return \em true on success
     *
     * Ex: addKeyUsage(
     *       "critical, digitalSignature, keyEncipherment, keyAgreement"
     *     );
     */
    bool addKeyUsage(const std::string& ku)
    {
      return addExt(NID_key_usage, ku);
    }

    /**
     * @brief   Add extended key usage
     * @param   eku Extended key usage string
     * @return  Return \em true on success
     *
     * Ex: addExtKeyUsage("serverAuth");
     */
    bool addExtKeyUsage(const std::string& eku)
    {
      return addExt(NID_ext_key_usage, eku);
    }

    /**
     * @brief   Add subject alternative names
     * @param   Subject alternative names string
     * @return  Return \em true on success
     *
     * Ex: addSUbjectAltNames("DNS:dom.org,IP:1.2.3.4,email:user@dom.org")
     */
    bool addSubjectAltNames(const std::string& san)
    {
      return addExt(NID_subject_alt_name, san);
    }

    /**
     * @brief   Get the subject alternative names extension
     * @return  Returns an object representing the SAN
     */
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

    /**
     * @brief   Add a subject alternative names object
     * @param   san A subject alternative names object containing names to add
     */
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

    /**
     * @brief   Cast to a pointer to a stack of X509_EXTENSION objects
     */
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
