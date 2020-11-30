#pragma once

#include <bytes/bytes.h>
#include <hpke/signature.h>

#include <memory>

using namespace bytes_ns;

namespace hpke {

struct Certificate
{

private:
  struct ParsedCertificate;
  std::unique_ptr<ParsedCertificate> parsed_cert;

public:
  explicit Certificate(const bytes& der);
  Certificate() = delete;
  Certificate(const Certificate& other);
  ~Certificate();

  bool valid_from(const Certificate& parent) const;

  // Accessors for parsed certificate elements
  bytes issuer() const;
  bytes subject() const;
  bool is_ca() const;
  bytes subject_key_id() const;
  bytes authority_key_id() const;
  std::vector<std::string> email_addresses() const;
  std::vector<std::string> dns_names() const;

  const Signature::ID public_key_algorithm;
  const std::unique_ptr<Signature::PublicKey> public_key;
  const bytes raw;
};

} // namespace hpke
