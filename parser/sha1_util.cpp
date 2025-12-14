#include "sha1_util.hpp"
#include "sha1.hpp"   // the big header-only SHA1 implementation

std::string sha1(const std::string &data)
{
    SHA1 sha;
    sha.update(data);
    return sha.final();
}
