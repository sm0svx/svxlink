#include <iostream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <iterator>

#include <AsyncDigest.h>

int main()
{
  const std::string msg("The quick brown fox jumps over the lazy dog");
  const std::string md_algorithm("sha256");
  const std::string private_key_pem =
      "-----BEGIN PRIVATE KEY-----\n"
      "MIIBVgIBADANBgkqhkiG9w0BAQEFAASCAUAwggE8AgEAAkEA9hMmwek/t6lsQ4P1\n"
      "mouGSfnKeeJKgQ7V10pF6eLbtgke5bGpvObJpmOC4rhBcvUWRM26fAtN28UB1uTs\n"
      "lQoprwIDAQABAkAqeE21I/uiSDRuRqUqAjCwLdN7S8oOEjBoEuKUJlpDRWMTmUIi\n"
      "jz5KUF8dKFUESIBr4wm0eFwvEQ3Hc0s+NOxZAiEA+2iLdJjOCIM1Cf/GWS+Zm7VE\n"
      "Jigcxb4lmkCp6SieMvUCIQD6katuM9cwxciRyxRw5//eIul75UF3W5YD9+O5uDzr\n"
      "kwIhAJAihtlJBc5hktXxuwDExnc7vB94Hc7MzfganI8dB13FAiEAzz85Kdda/443\n"
      "jM8JwzFA4rzBnaZLdauc8v9PrccDLF0CIQD4IW5bAfBBCNozBj1937NPMGNF+Jdf\n"
      "5bCIctxqutH20w==\n"
      "-----END PRIVATE KEY-----\n";
  const std::string public_key_pem =
      "-----BEGIN PUBLIC KEY-----\n"
      "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAPYTJsHpP7epbEOD9ZqLhkn5ynniSoEO\n"
      "1ddKReni27YJHuWxqbzmyaZjguK4QXL1FkTNunwLTdvFAdbk7JUKKa8CAwEAAQ==\n"
      "-----END PUBLIC KEY-----\n";

  Async::SslKeypair private_key;
  if (!private_key.privateKeyFromPem(private_key_pem))
  {
    std::cout << "*** ERROR: Async::Digest::privateKeyFromPem() failed"
              << std::endl;
    return 1;
  }
  std::cout << private_key.privateKeyPem() << std::endl;

  Async::SslKeypair public_key;
  if (!public_key.publicKeyFromPem(public_key_pem))
  {
    std::cout << "*** ERROR: Async::Digest::publicKeyFromPem() failed"
              << std::endl;
    return 1;
  }
  std::cout << public_key.publicKeyPem() << std::endl;

  Async::Digest sdgst;
  if (!sdgst.signInit(md_algorithm, private_key))
  {
    std::cout << "*** ERROR: Async::Digest::signInit() failed" << std::endl;
    return 1;
  }
  const auto sig = sdgst.sign(msg);
  if (sig.empty())
  {
    std::cout << "*** ERROR: Async::Digest::sign() failed" << std::endl;
    return 1;
  }
  std::cout << "Signature size: " << sig.size() << std::endl;
  size_t cnt = 0;
  for (const auto& byte : sig)
  {
    std::cout << std::setfill('0') << std::setw(2) << std::hex
              << unsigned(byte) << " ";
    if (++cnt % 16 == 0)
    {
      std::cout << std::endl;
    }
  }
  std::cout << std::endl;

  Async::Digest vdgst;
  if (!vdgst.signVerifyInit(md_algorithm, public_key))
  {
    std::cout << "*** ERROR: Async::Digest::signVerifyInit() failed"
              << std::endl;
    return 1;
  }
  bool verify_ok = vdgst.signVerify(sig, msg);
  std::cout << "Verify: " << (verify_ok ? "OK" : "FAIL") << "\n" << std::endl;


  Async::Digest dgst;
  if (!dgst.mdInit("sha256"))
  {
    std::cout << "*** ERROR: Async::Digest::init() failed"
              << std::endl;
    return 1;
  }
  if (!dgst.mdUpdate(msg))
  {
    std::cout << "*** ERROR: Async::Digest::update() failed"
              << std::endl;
    return 1;
  }
  const auto sha256sum = dgst.mdFinal();
  if (sha256sum.empty())
  {
    std::cout << "*** ERROR: Async::Digest::final() failed"
              << std::endl;
    return 1;
  }
  std::cout << "SHA256SUM:" << std::endl;
  cnt = 0;
  for (const auto& byte : sha256sum)
  {
    std::cout << std::setfill('0') << std::setw(2) << std::hex
              << unsigned(byte) << " ";
    if (++cnt % 16 == 0)
    {
      std::cout << std::endl;
    }
  }
  std::cout << std::endl;

  return 0;
}
