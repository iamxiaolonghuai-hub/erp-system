#include "Password.h"

#include <openssl/sha.h>
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>

namespace erp
{

std::string Password::sha256(const std::string &data)
{
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(data.data()), data.size(),
           digest);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        oss << std::setw(2) << static_cast<int>(digest[i]);
    return oss.str();
}

std::string Password::randomSaltHex(size_t bytes)
{
    unsigned char buf[64];
    if (bytes > sizeof(buf))
        bytes = sizeof(buf);
    RAND_bytes(buf, static_cast<int>(bytes));
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < bytes; ++i)
        oss << std::setw(2) << static_cast<int>(buf[i]);
    return oss.str();
}

std::string Password::hash(const std::string &plain)
{
    std::string salt = randomSaltHex();
    std::string h = sha256(salt + plain);
    return salt + "$" + h;
}

bool Password::verify(const std::string &plain, const std::string &hashed)
{
    size_t pos = hashed.find('$');
    if (pos == std::string::npos)
        return false;
    std::string salt = hashed.substr(0, pos);
    std::string h = sha256(salt + plain);
    return h == hashed.substr(pos + 1);
}

}  // namespace erp
