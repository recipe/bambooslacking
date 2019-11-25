#pragma once

#include <string>

namespace bs {
/// @brief Calculates HMAC_SHA256 hash
/// @param data A data to hash
/// @param key A key
/// @returns hash as a string
std::string hmac_sha256(const std::string& data, const std::string& key);

/// @brief Encrypts data using AES_cfb8_encrypt algorithm
/// @param data A string to encrypt
/// @param key A secret key. The size must be 32 characters or more
/// @returns Returns base64 encoded encrypted string
std::string encrypt(const std::string& data, const std::string& key);

/// @brief Decrypts data using AES_cfb8_encrypt algorithm
/// @param data An base64 encoded binary string to decrypt
/// @param key A secret key. The size must be 32 characters or more
/// @returns Returns decrypted value
std::string decrypt(const std::string& data, const std::string& key);

/// @brief helper function that creates a real key for using in encryption
/// @param key An original key string
/// @param key_length A length of the original key
/// @param rkey A real key that is being created
/// @param key_size A length of the key that should be used for the specified cipher algorithm
/// @param iv Initialization vector to set
/// @param iv_size A length of the IV that should be used for the specified cipher algorithm
void create_key(const unsigned char* key,
                const uint8_t key_length,
                uint8_t* rkey,
                const uint8_t key_size,
                uint8_t* iv,
                const uint8_t iv_size);
} // namespace bs