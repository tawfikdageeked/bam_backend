#pragma once
#include <sodium.h>
#include <stdexcept>
#include <string>




namespace Crypto
{
    // initialize the encrypter
    void Init();

    // takes a plain password and hashs it and retunred the hashed string
    std::string HashPassword(const std::string& plain_password);


    // takes a plain password from the http request and compares it to the hashed password and return a bool
    bool VerifyPassword(const std::string& db_hash, const std::string& incoming_password);
}
