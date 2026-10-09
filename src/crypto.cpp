#include "crypto.hpp"

namespace Crypto
{
    void Init()
    {
        if(sodium_init() < 0)
        {
            throw std::runtime_error("Failed to initialize encrypter");
        }
    }

    std::string HashPassword(const std::string& plain_password)
    {
        char hashed_buffer[crypto_pwhash_STRBYTES];

        if(crypto_pwhash_str(
            hashed_buffer,
            plain_password.c_str(),
            plain_password.length(),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE
        ) != 0)
        {
            throw std::runtime_error("Out of memory during password hashing");
        }

        return std::string(hashed_buffer);
    }

    bool VerifyPassword(const std::string& db_hash, const std::string& incoming_password)
    {
        return crypto_pwhash_str_verify(
            db_hash.c_str(),
            incoming_password.c_str(),
            incoming_password.length()
        ) == 0;
    }
}
