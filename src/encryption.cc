#include <iomanip>
#include <cstring>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include "base64.h"
#include "encryption.h"

namespace bs {
std::string hmac_sha256(const std::string& data, const std::string& key) {
  unsigned int diglen;
  unsigned char result[EVP_MAX_MD_SIZE];

  auto digest = HMAC(
      EVP_sha256(),
      reinterpret_cast<const unsigned char*>(key.c_str()), key.length(),
      reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
      result,
      &diglen
  );

  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (int i = 0; i < 32; i++) {
    ss << std::hex << std::setw(2) << (unsigned int)digest[i];
  }

  return ss.str();
}

void create_key(
    const unsigned char* key,
    const uint8_t key_length,
    uint8_t* rkey,
    const uint8_t key_size,
    uint8_t* iv,
    const uint8_t iv_size
) {
  uint8_t *rkey_end;
  uint8_t *ptr;
  const uint8_t *sptr;
  const uint8_t *key_end = key + key_length;

  rkey_end = rkey + key_size;

  memset(rkey, 0, key_size);
  memset(iv, 0, iv_size);

  // initializing real key
  for (ptr = rkey, sptr = key; sptr < key_end; ptr++, sptr++) {
    if (ptr == rkey_end) {
      ptr = rkey;
    }
    *ptr ^= *sptr;
  }

  // composing IV
  for (ptr = iv, sptr = key_end - (key_length > iv_size ? iv_size : key_length); sptr < key_end; ptr++, sptr++) {
    *ptr = *sptr;
  }
}

std::string encrypt(const std::string& data, const std::string& secret)
{
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  const EVP_CIPHER *cipher = EVP_aes_256_cfb8();
  uint8_t key_length = EVP_CIPHER_key_length(cipher);
  uint8_t block_size = EVP_CIPHER_block_size(cipher);
  uint8_t iv_length = EVP_CIPHER_iv_length(cipher);
  int u_len, f_len;
  unsigned char rkey[key_length];
  unsigned char iv[iv_length];
  // calculate the length of encoded string
  int dest_length = block_size > 1 ? block_size * (data.length() / block_size) + block_size : data.length();
  // encoded string result
  unsigned char dest[dest_length];
  memset(dest, 0, dest_length);

  // Creates a key of the specific length for this type of the cipher algorithm
  create_key(reinterpret_cast<const unsigned char*>(secret.c_str()), secret.length(), rkey, key_length, iv, iv_length);

  EVP_EncryptInit(ctx, cipher, rkey, iv);
  EVP_CIPHER_CTX_set_padding(ctx, true);
  EVP_EncryptUpdate(ctx, dest, &u_len, reinterpret_cast<const unsigned char*>(data.c_str()), data.length());
  EVP_EncryptFinal(ctx, dest + u_len, &f_len);
  EVP_CIPHER_CTX_free(ctx);

  if (u_len + f_len != dest_length) {
    throw std::runtime_error("Unable to encrypt string. Unexpected result length.");
  };

  std::stringstream ss;
  for (int i = 0; i < dest_length; i++) {
    ss << dest[i];
  }

  return base64_encode(ss.str());
}

std::string decrypt(const std::string& data, const std::string& secret)
{
  const std::string b64decoded = base64_decode(data);
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  const EVP_CIPHER *cipher = EVP_aes_256_cfb8();
  uint8_t key_length = EVP_CIPHER_key_length(cipher);
  uint8_t iv_length = EVP_CIPHER_iv_length(cipher);
  int u_len, f_len;
  unsigned char rkey[key_length];
  unsigned char iv[iv_length];
  unsigned char dest[b64decoded.size()];
  memset(dest, 0, b64decoded.size());

  // Creates a key of the specific length for this type of the cipher algorithm
  create_key(reinterpret_cast<const unsigned char*>(secret.c_str()), secret.length(), rkey, key_length, iv, iv_length);

  EVP_DecryptInit(ctx, cipher, rkey, iv);
  EVP_CIPHER_CTX_set_padding(ctx, true);
  EVP_DecryptUpdate(ctx, dest, &u_len, reinterpret_cast<const unsigned char*>(b64decoded.data()), b64decoded.size());
  EVP_DecryptFinal_ex(ctx, dest + u_len, &f_len);
  EVP_CIPHER_CTX_free(ctx);

  std::stringstream ss;
  for (int i = 0; i < (u_len + f_len); i++) {
    ss << dest[i];
  }

  return ss.str();
}
} // namespace bs