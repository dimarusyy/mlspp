#include "openssl_common.h"

#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace hpke {

template<>
void
typed_delete(EVP_CIPHER_CTX* ptr)
{
  EVP_CIPHER_CTX_free(ptr);
}

template<>
void
typed_delete(EVP_PKEY_CTX* ptr)
{
  EVP_PKEY_CTX_free(ptr);
}

template<>
void
typed_delete(EVP_PKEY* ptr)
{
  EVP_PKEY_free(ptr);
}

template<>
void
typed_delete(BIGNUM* ptr)
{
  BN_free(ptr);
}

template<>
void
typed_delete(EC_POINT* ptr)
{
  EC_POINT_free(ptr);
}

template<>
void
typed_delete(EC_KEY* ptr)
{
  EC_KEY_free(ptr);
}

///
/// Map OpenSSL errors to C++ exceptions
///

std::runtime_error
openssl_error()
{
  auto code = ERR_get_error();
  return std::runtime_error(ERR_error_string(code, nullptr));
}

///
/// Random data
///

bytes
random_bytes(size_t size)
{
  auto rand = bytes(size);
  if (1 != RAND_bytes(rand.data(), size)) {
    throw openssl_error();
  }
  return rand;
}

} // namespace hpke