#include <AsyncSslContext.h>
#include <AsyncSslKeypair.h>
#include <AsyncSslCertSigningReq.h>
#include <AsyncSslX509.h>

int main(void)
{
    // Create a key pair for the CA
  Async::SslKeypair ca_pkey;
  if (!ca_pkey.generate(2048))
  {
    std::cout << "*** ERROR: Failed to generate CA key" << std::endl;
    return 1;
  }
  if (!ca_pkey.writePrivateKeyFile("demo_ca.key"))
  {
    std::cout << "*** WARNING: Failed to write CA key file" << std::endl;
  }

    // Create a CA certificate and sign it with the key above
  Async::SslX509 ca_cert;
  ca_cert.setSerialNumber(1);
  ca_cert.setVersion(Async::SslX509::VERSION_3);
  ca_cert.addIssuerName("CN", "Demo Root CA");
  ca_cert.addIssuerName("L", "My City");
  ca_cert.addIssuerName("C", "XX");
  ca_cert.setSubjectName(ca_cert.issuerName());
  Async::SslX509Extensions ca_exts;
  ca_exts.addBasicConstraints("critical, CA:TRUE");
  ca_exts.addKeyUsage("critical, cRLSign, digitalSignature, keyCertSign");
  ca_exts.addSubjectAltNames("email:ca@example.org");
  ca_cert.addExtensions(ca_exts);
  time_t t = time(nullptr);
  ca_cert.setNotBefore(t);
  ca_cert.setNotAfter(t + 24*3600);
  ca_cert.setPublicKey(ca_pkey);
  ca_cert.sign(ca_pkey);
  std::cout << "--------------- CA Certificate ----------------" << std::endl;
  ca_cert.print();
  std::cout << "-----------------------------------------------" << std::endl;
  if (!ca_cert.writePemFile("demo_ca.crt"))
  {
    std::cout << "*** WARNING: Failed to write CA certificate file"
              << std::endl;
  }

    // Create a key pair for the server certificate
  Async::SslKeypair cert_pkey;
  if (!cert_pkey.generate(2048))
  {
    std::cout << "*** ERROR: Failed to generate server certificate key"
              << std::endl;
    return 1;
  }
  if (!cert_pkey.writePrivateKeyFile("demo.key"))
  {
    std::cout << "*** WARNING: Failed to write CA key file" << std::endl;
  }

    // Create a Certificate Signing Request
  Async::SslCertSigningReq csr;
  csr.setVersion(Async::SslCertSigningReq::VERSION_1);
  csr.addSubjectName("CN", "hostname.example.org");
  csr.addSubjectName("L", "My City");
  csr.addSubjectName("C", "XX");
  Async::SslX509Extensions csr_exts;
  csr_exts.addSubjectAltNames(
      "DNS:hostname.example.org"
      ", DNS:alias.example.org"
      ", DNS:localhost"
      ", IP:127.0.0.1"
      ", email:admin@example.org"
      ", URI:https://www.example.org"
      ", otherName:msUPN;UTF8:sb@sb.local");
  csr.addExtensions(csr_exts);
  csr.setPublicKey(cert_pkey);
  csr.sign(cert_pkey);
  std::cout << "--------- Certificate Signing Request ---------" << std::endl;
  csr.print();
  std::cout << "-----------------------------------------------" << std::endl;
  if (!csr.writePemFile("demo.csr"))
  {
    std::cout << "*** WARNING: Failed to write CSR file" << std::endl;
  }
  std::cout << "The CSR verification "
            << (csr.verify(cert_pkey) ? "PASSED" : "FAILED")
            << std::endl;

    // Create the certificate using the CSR then sign it using the CA cert
  Async::SslX509 cert;
  cert.setSerialNumber(2);
  cert.setVersion(Async::SslX509::VERSION_3);
  cert.setIssuerName(ca_cert.subjectName());
  cert.setSubjectName(csr.subjectName());
  cert.setNotBefore(t);
  cert.setNotAfter(t + 3600);
  const Async::SslX509Extensions exts(csr.extensions());
  Async::SslX509Extensions cert_exts;
  cert_exts.addBasicConstraints("critical, CA:FALSE");
  cert_exts.addKeyUsage("critical, nonRepudiation, digitalSignature, keyEncipherment, keyAgreement");
  cert_exts.addExtKeyUsage("serverAuth");
  Async::SslX509ExtSubjectAltName san(exts.subjectAltName());
  cert_exts.addExtension(san);
  cert.addExtensions(cert_exts);
  Async::SslKeypair csr_pkey(csr.publicKey());
  cert.setPublicKey(csr_pkey);
  cert.sign(ca_pkey);
  std::cout << "------------- Server Certificate --------------" << std::endl;
  cert.print();
  std::cout << "-----------------------------------------------" << std::endl;
  if (!cert.writePemFile("demo.crt"))
  {
    std::cout << "*** WARNING: Failed to write certificate file"
              << std::endl;
  }
  std::cout << "The certificate verification "
            << (cert.verify(ca_pkey) ? "PASSED" : "FAILED")
            << std::endl;

  return 0;
}
