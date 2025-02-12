/**
@file   AsyncSslContext.h
@brief  SSL context meant to be used with TcpConnection and friends
@author Tobias Blomberg / SM0SVX
@date   2020-08-01

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2020 Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_SSL_CONTEXT
#define ASYNC_SSL_CONTEXT


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

#include <iostream>
#include <cassert>


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
@brief  SSL context meant to be used with TcpConnection and friends
@author Tobias Blomberg / SM0SVX
@date   2020-08-01

A_detailed_class_description

\include MyNamespaceTemplate_demo.cpp
*/
class SslContext
{
  public:
    /**
     * @brief   Default constructor
     */
    SslContext(void)
    {
      initializeGlobals();

        // Create the SSL server context
      //m_ctx = SSL_CTX_new(SSLv23_method());
      m_ctx = SSL_CTX_new(TLS_method());
      assert(m_ctx != nullptr);

        // Recommended to avoid SSLv2 & SSLv3
      //SSL_CTX_set_options(m_ctx, SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);
      SSL_CTX_set_options(m_ctx, SSL_OP_ALL);
      SSL_CTX_set_min_proto_version(m_ctx, TLS1_2_VERSION);

      SSL_CTX_set_verify(m_ctx, SSL_VERIFY_PEER, NULL);

        // Set up OpenSSL to look for CA certs in the default locations
      SSL_CTX_set_default_verify_paths(m_ctx);
      //int SSL_CTX_set_default_verify_dir(SSL_CTX *ctx);
      //int SSL_CTX_set_default_verify_file(SSL_CTX *ctx);
    }

    /**
     * @brief   Constructor
     * @param   ctx Use this existing context
     */
    SslContext(SSL_CTX* ctx) : m_ctx(ctx) {}

    /**
     * @brief   Do not allow copy construction
     */
    SslContext(const SslContext&) = delete;

    /**
     * @brief   Do not allow assignment
     */
    SslContext& operator=(const SslContext&) = delete;

    /**
     * @brief   Destructor
     */
    ~SslContext(void)
    {
      SSL_CTX_free(m_ctx);
      m_ctx = nullptr;
    }

    /**
     * @brief   A_brief_member_function_description
     * @param   param1 Description_of_param1
     * @return  Return_value_of_this_member_function
     */
    bool setCertificateFiles(const std::string& keyfile,
                             const std::string& crtfile)
    {
      if (crtfile.empty() || keyfile.empty()) return false;

        // Load certificate chain and private key files, and check consistency
      //if (SSL_CTX_use_certificate_file(
      //      m_ctx, crtfile.c_str(), SSL_FILETYPE_PEM) != 1)
      if (SSL_CTX_use_certificate_chain_file(m_ctx, crtfile.c_str()) != 1)
      {
        sslPrintErrors("SSL_CTX_use_certificate_chain_file failed");
        return false;
      }

      if (SSL_CTX_use_PrivateKey_file(
            m_ctx, keyfile.c_str(), SSL_FILETYPE_PEM) != 1)
      {
        sslPrintErrors("SSL_CTX_use_PrivateKey_file failed");
        return false;
      }

        // Make sure that the key and certificate files match
      if (SSL_CTX_check_private_key(m_ctx) != 1)
      {
        sslPrintErrors("SSL_CTX_check_private_key failed");
        return false;
      }
      //std::cout << "### SslContext::setCertificateFiles: "
      //             "Certificate and private key loaded and verified"
      //          << std::endl;

      return true;
    }

    bool caCertificateFileIsSet(void) const { return m_cafile_set; }

    bool setCaCertificateFile(const std::string& cafile)
    {
      int ret = SSL_CTX_load_verify_locations(m_ctx, cafile.c_str(), NULL);
      m_cafile_set = (ret == 1);
      return m_cafile_set;
    }

    void sslPrintErrors(const char* fname)
    {
      std::cerr << "*** ERROR: OpenSSL failed: ";
      ERR_print_errors_fp(stderr);
    } /* sslPrintErrors */

    operator SSL_CTX*(void) { return m_ctx; }
    operator const SSL_CTX*(void) const { return m_ctx; }

  protected:

  private:
    SSL_CTX*  m_ctx         = nullptr;
    bool      m_cafile_set  = false;

    static void initializeGlobals(void)
    {
      static bool is_initialized = false;
      if (!is_initialized)
      {
        SSL_library_init();
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
#if OPENSSL_VERSION_MAJOR < 3
        ERR_load_BIO_strings();
#endif
        ERR_load_crypto_strings();
#endif
        is_initialized = true;
      }
    }

};  /* class SslContext */


} /* namespace */

#endif /* ASYNC_SSL_CONTEXT */

/*
 * This file has not been truncated
 */
