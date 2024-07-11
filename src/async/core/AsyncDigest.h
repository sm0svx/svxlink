/**
@file   AsyncDigest.h
@brief  A_brief_description_for_this_file
@author Tobias Blomberg / SM0SVX
@date   2024-04-27

\verbatim
<A brief description of the program or library this file belongs to>
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

/** @example AsyncDigest_demo.cpp
An example of how to use the Async::Digest class
*/

#ifndef ASYNC_DIGEST_INCLUDED
#define ASYNC_DIGEST_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include <vector>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncSslKeypair.h>


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
@date   2024-04-27

A_detailed_class_description

\include AsyncDigest_demo.cpp
*/
class Digest
{
  public:
    using Signature = std::vector<uint8_t>;
    using MsgDigest = std::vector<uint8_t>;

    static bool sigEqual(const Signature& s1, const Signature& s2)
    {
      return (s1.size() == s2.size()) &&
             (CRYPTO_memcmp(s1.data(), s2.data(), s1.size()) == 0);
    }

    /**
     * @brief   Default constructor
     */
    Digest(void)
    {
#if OPENSSL_VERSION_MAJOR < 3
      static bool global_is_initialized = false;
      if (!global_is_initialized)
      {
        //std::cout << "### Digest::Digest: OpenSSL_add_all_digests"
        //          << std::endl;
        OpenSSL_add_all_digests();
        global_is_initialized = true;
      }
#endif
      m_ctx = EVP_MD_CTX_new();
      if (m_ctx == nullptr)
      {
        std::cerr << "*** ERROR: EVP_MD_CTX_new failed, error "
                  << ERR_get_error() << std::endl;
        abort();
      }
    }

    /**
     * @brief   Disallow copy construction
     */
    Digest(const Digest&) = delete;

    /**
     * @brief   Disallow copy assignment
     */
    Digest& operator=(const Digest&) = delete;

    /**
     * @brief   Destructor
     */
    ~Digest(void)
    {
      EVP_MD_CTX_free(m_ctx);
      m_ctx = nullptr;
    }

    /**
     * @brief   A_brief_member_function_description
     * @param   param1 Description_of_param1
     * @return  Return_value_of_this_member_function
     */

    bool mdInit(const std::string& md_alg)
    {
      MessageDigest md(md_alg);
      if (md == nullptr)
      {
        std::cerr << "*** ERROR: EVP_MD_fetch failed, error "
                  << ERR_get_error() << std::endl;
        return false;
      }
      int rc = EVP_DigestInit_ex(m_ctx, md, nullptr);
      //EVP_MD_free(md);
      if (rc != 1)
      {
        std::cerr << "*** ERROR: EVP_DigestInit_ex failed, error "
                  << ERR_get_error() << std::endl;
        return false;
      }
      return true;
    }

    bool mdUpdate(const void* d, size_t dlen)
    {
      int rc = EVP_DigestUpdate(m_ctx, d, dlen);
      if (rc != 1)
      {
        std::cerr << "*** ERROR: EVP_DigestUpdate failed, error "
                  << ERR_get_error() << std::endl;
        return false;
      }
      return true;
    }

    template <class T>
    bool mdUpdate(const T& d)
    {
      return mdUpdate(d.data(), d.size());
    }

    bool mdFinal(MsgDigest& md)
    {
      unsigned int mdlen = EVP_MAX_MD_SIZE;
      md.resize(mdlen);
      int rc = EVP_DigestFinal_ex(m_ctx, md.data(), &mdlen);
      if (rc != 1)
      {
        std::cerr << "*** ERROR: EVP_DigestFinal_ex failed, error "
                  << ERR_get_error() << std::endl;
        md.clear();
        return false;
      }
      md.resize(mdlen);
      return true;
    }

    MsgDigest mdFinal(void)
    {
      MsgDigest digest;
      (void)mdFinal(digest);
      return digest;
    }

    bool md(MsgDigest& digest, const std::string& md_alg,
           const void* d, size_t dlen)
    {
      return mdInit(md_alg) && mdUpdate(d, dlen) && mdFinal(digest);
    }

    template <class T>
    bool md(MsgDigest& digest, const std::string& md_alg, const T& d)
    {
      return md(digest, md_alg, d.data(), d.size());
    }

    template <class T>
    MsgDigest md(const std::string& md_alg, const T& d)
    {
      MsgDigest digest;
      (void)md(digest, md_alg, d);
      return digest;
    }


    bool signInit(const std::string& md_alg, SslKeypair& pkey)
    {
      //EVP_MD* md = EVP_MD_fetch(nullptr, md_alg.c_str(), nullptr);
      MessageDigest md(md_alg);
      if (md == nullptr)
      {
        std::cerr << "*** ERROR: EVP_MD_fetch failed, error "
                  << ERR_get_error() << std::endl;
        return false;
      }
      int rc = EVP_DigestSignInit(m_ctx, NULL, md, NULL, pkey);
      //EVP_MD_free(md);
      if (rc != 1)
      {
        std::cerr << "*** ERROR: EVP_DigestSignInit failed, error "
                  << ERR_get_error() << std::endl;
        return false;
      }
      return true;
    }

    bool signUpdate(const void* msg, size_t mlen)
    {
      int rc = EVP_DigestSignUpdate(m_ctx,
          reinterpret_cast<const unsigned char*>(msg), mlen);
      if (rc != 1)
      {
        std::cerr << "*** ERROR: EVP_DigestSignUpdate failed, error "
                  << ERR_get_error() << std::endl;
        return false;
      }
      return true;
    }

    template <class T>
    bool signUpdate(const T& msg)
    {
      return signUpdate(msg.data(), msg.size());
    }

    bool signFinal(Signature& sig)
    {
      sig.clear();
      size_t req = 0;
      int rc = EVP_DigestSignFinal(m_ctx, NULL, &req);
      if (rc != 1)
      {
        std::cerr << "*** ERROR: EVP_DigestSignFinal (1) failed , error "
                  << ERR_get_error() << std::endl;
        return false;
      }
      sig.resize(req);
      rc = EVP_DigestSignFinal(m_ctx, sig.data(), &req);
      if (rc != 1)
      {
        std::cerr << "*** ERROR: EVP_DigestSignFinal (2) failed, error "
                  << ERR_get_error() << std::endl;
        sig.clear();
        return false;
      }
      return true;
    }

    Signature signFinal(void)
    {
      Signature sig;
      (void)signFinal(sig);
      return sig;
    }

    bool sign(Signature& sig, const void* msg, size_t mlen)
    {
      sig.clear();

#if OPENSSL_VERSION_MAJOR >= 3
      size_t siglen = 0;
      int rc = EVP_DigestSign(m_ctx, nullptr, &siglen,
          reinterpret_cast<const unsigned char*>(msg), mlen);
      if (rc != 1)
      {
        std::cerr << "*** ERROR: EVP_DigestSign (1) failed, error "
                  << ERR_get_error() << std::endl;
        return false;
      }
      sig.resize(siglen);
      rc = EVP_DigestSign(m_ctx, sig.data(), &siglen,
          reinterpret_cast<const unsigned char*>(msg), mlen);
      if (rc != 1)
      {
        std::cerr << "*** ERROR: EVP_DigestSign (2) failed, error "
                  << ERR_get_error() << std::endl;
        sig.clear();
        return false;
      }
      return true;
#else
      return signUpdate(msg, mlen) && signFinal(sig);
#endif
    }

    template <class T>
    bool sign(Signature& sig, const T& msg)
    {
      return sign(sig, msg.data(), msg.size());
    }

    Signature sign(const void* msg, size_t mlen)
    {
      Signature sig;
      (void)sign(sig, msg, mlen);
      return sig;
    }

    template <class T>
    Signature sign(const T& msg)
    {
      return sign(msg.data(), msg.size());
    }

    bool signVerifyInit(const std::string& md_alg, SslKeypair& pkey)
    {
      assert(!pkey.isNull());
      //EVP_MD* md = EVP_MD_fetch(nullptr, md_alg.c_str(), nullptr);
      MessageDigest md(md_alg);
      if (md == nullptr)
      {
        return false;
      }
      int rc = EVP_DigestVerifyInit(m_ctx, NULL, md, NULL, pkey);
      //EVP_MD_free(md);
      if (rc != 1)
      {
        std::cerr << "*** ERROR: EVP_DigestVerifyInit failed, error "
                  << ERR_get_error() << std::endl;
        return false;
      }
      return true;
    }

    bool signVerifyUpdate(const void* msg, size_t mlen)
    {
      assert((msg != nullptr) && (mlen > 0));
      int rc = EVP_DigestVerifyUpdate(m_ctx,
          reinterpret_cast<const unsigned char*>(msg), mlen);
      if (rc != 1)
      {
        std::cerr << "*** ERROR: EVP_DigestVerifyUpdate failed, error "
                  << ERR_get_error() << std::endl;
        return false;
      }
      return true;
    }

    template <class T>
    bool signVerifyUpdate(const T& msg)
    {
      return signVerifyUpdate(msg.data(), msg.size());
    }

    bool signVerifyFinal(const Signature& sig)
    {
      int rc = EVP_DigestVerifyFinal(m_ctx, sig.data(), sig.size());
      return (rc == 1);
    }

    bool signVerify(const Signature& sig, const void* msg, size_t mlen)
    {
#if OPENSSL_VERSION_MAJOR >= 3
      int rc = EVP_DigestVerify(m_ctx, sig.data(), sig.size(),
          reinterpret_cast<const unsigned char*>(msg), mlen);
      return (rc == 1);
#else
      return signVerifyUpdate(msg, mlen) && signVerifyFinal(sig);
#endif
    }

    template <class T>
    bool signVerify(const Signature& sig, const T& msg)
    {
      return signVerify(sig, msg.data(), msg.size());
    }

  protected:

  private:
    class MessageDigest
    {
      public:
        MessageDigest(const std::string& md_alg)
        {
#if OPENSSL_VERSION_MAJOR >= 3
          m_md = EVP_MD_fetch(nullptr, md_alg.c_str(), nullptr);
#else
          m_md = EVP_get_digestbyname(md_alg.c_str());
#endif
        }
        ~MessageDigest(void)
        {
#if OPENSSL_VERSION_MAJOR >= 3
          EVP_MD_free(m_md);
#endif
          m_md = nullptr;
        }
        operator const EVP_MD*() const { return m_md; }
        bool operator==(nullptr_t) const { return (m_md == nullptr); }
      private:
#if OPENSSL_VERSION_MAJOR >= 3
        EVP_MD* m_md = nullptr;
#else
        const EVP_MD* m_md = nullptr;
#endif
    };

    EVP_MD_CTX*   m_ctx = nullptr;

};  /* class Digest */


} /* namespace Async */

#endif /* ASYNC_DIGEST_INCLUDED */

/*
 * This file has not been truncated
 */
