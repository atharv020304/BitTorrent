#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

// Forward declaration
struct BValue;

// Shared pointer alias
using BPtr = std::shared_ptr<BValue>;

/*
 * Represents a generic Bencode value:
 *  - Integer
 *  - String
 *  - List
 *  - Dictionary
 */
struct BValue
{
    enum class Type
    {
        Integer,
        String,
        List,
        Dict
    };

    Type type;
    long long ival = 0;
    std::string s;
    std::vector<BPtr> list;
    std::map<std::string, BPtr> dict;

    // Constructors
    explicit BValue(long long value);
    BValue(const std::string &str, bool isString);
    explicit BValue(Type t);
};

/* =========================
   Public API
   ========================= */

// Decode bencoded buffer into AST
BPtr bdecode(const std::string &buf);

// Encode AST back into bencode string
std::string bencode(const BPtr &node);

// Dictionary helper
BPtr dict_get(BPtr dict, const std::string &key);

// SHA1 helper (used by torrent info-hash)
std::string sha1(const std::string &data);

// Optional: File helper
std::string readFile(const std::string &path);
