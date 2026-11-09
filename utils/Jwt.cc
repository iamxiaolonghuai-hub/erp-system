#include "Jwt.h"

#include <openssl/hmac.h>
#include <json/json.h>
#include <ctime>
#include <sstream>

namespace erp
{

static const char *kDefaultSecret = "erp-system-jwt-secret-please-change-in-production";

std::string &Jwt::secret()
{
    static std::string s = kDefaultSecret;
    return s;
}

void Jwt::setSecret(const std::string &s)
{
    secret() = s;
}

std::string Jwt::base64UrlEncode(const unsigned char *data, size_t len)
{
    static const char *table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3)
    {
        unsigned int n = data[i] << 16;
        if (i + 1 < len)
            n |= data[i + 1] << 8;
        if (i + 2 < len)
            n |= data[i + 2];

        out.push_back(table[(n >> 18) & 0x3F]);
        out.push_back(table[(n >> 12) & 0x3F]);
        out.push_back(i + 1 < len ? table[(n >> 6) & 0x3F] : '=');
        out.push_back(i + 2 < len ? table[n & 0x3F] : '=');
    }
    // URL-safe 替换
    for (auto &c : out)
    {
        if (c == '+')
            c = '-';
        else if (c == '/')
            c = '_';
    }
    // 去掉末尾 '='
    while (!out.empty() && out.back() == '=')
        out.pop_back();
    return out;
}

std::string Jwt::base64UrlDecode(const std::string &input)
{
    std::string s = input;
    for (auto &c : s)
    {
        if (c == '-')
            c = '+';
        else if (c == '_')
            c = '/';
    }
    while (s.size() % 4 != 0)
        s.push_back('=');

    static const auto val = [](char c) -> int {
        if (c >= 'A' && c <= 'Z')
            return c - 'A';
        if (c >= 'a' && c <= 'z')
            return c - 'a' + 26;
        if (c >= '0' && c <= '9')
            return c - '0' + 52;
        if (c == '+')
            return 62;
        if (c == '/')
            return 63;
        return -1;
    };

    std::string out;
    int buf = 0, bits = 0;
    for (char c : s)
    {
        if (c == '=')
            break;
        int v = val(c);
        if (v < 0)
            continue;
        buf = (buf << 6) | v;
        bits += 6;
        if (bits >= 8)
        {
            bits -= 8;
            out.push_back(static_cast<char>((buf >> bits) & 0xFF));
        }
    }
    return out;
}

std::string Jwt::hmacSha256(const std::string &data)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;
    HMAC(EVP_sha256(), secret().data(), static_cast<int>(secret().size()),
         reinterpret_cast<const unsigned char *>(data.data()), data.size(),
         digest, &digestLen);
    return base64UrlEncode(digest, digestLen);
}

std::string Jwt::generate(const std::map<std::string, std::string> &payload,
                          int64_t expireSeconds)
{
    Json::Value header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    Json::Value body;
    for (const auto &kv : payload)
        body[kv.first] = kv.second;
    body["iat"] = static_cast<Json::Int64>(std::time(nullptr));
    body["exp"] =
        static_cast<Json::Int64>(std::time(nullptr) + expireSeconds);

    Json::StreamWriterBuilder wb;
    wb["indentation"] = "";
    std::string headerJson = Json::writeString(wb, header);
    std::string bodyJson = Json::writeString(wb, body);

    std::string headerB64 = base64UrlEncode(
        reinterpret_cast<const unsigned char *>(headerJson.data()),
        headerJson.size());
    std::string bodyB64 = base64UrlEncode(
        reinterpret_cast<const unsigned char *>(bodyJson.data()),
        bodyJson.size());

    std::string signInput = headerB64 + "." + bodyB64;
    std::string signature = hmacSha256(signInput);

    return signInput + "." + signature;
}

std::map<std::string, std::string> Jwt::verify(const std::string &token)
{
    std::map<std::string, std::string> result;

    size_t p1 = token.find('.');
    size_t p2 = token.find('.', p1 + 1);
    if (p1 == std::string::npos || p2 == std::string::npos)
        return result;

    std::string headerB64 = token.substr(0, p1);
    std::string bodyB64 = token.substr(p1 + 1, p2 - p1 - 1);
    std::string signature = token.substr(p2 + 1);

    // 验签
    std::string expected = hmacSha256(headerB64 + "." + bodyB64);
    if (expected != signature)
        return result;

    // 解析 payload
    std::string bodyJson = base64UrlDecode(bodyB64);
    Json::Value body;
    Json::CharReaderBuilder rb;
    std::string errs;
    std::istringstream iss(bodyJson);
    if (!Json::parseFromStream(rb, iss, &body, &errs))
        return result;

    // 校验过期
    if (body.isMember("exp"))
    {
        int64_t exp = body["exp"].asInt64();
        if (std::time(nullptr) > exp)
            return result;
    }

    for (const auto &key : body.getMemberNames())
    {
        if (body[key].isString())
            result[key] = body[key].asString();
    }
    return result;
}

}  // namespace erp
