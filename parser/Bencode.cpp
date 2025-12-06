#include <bits/stdc++.h>
#include "sha1.hpp"
using namespace std;

struct BValue;
using BPtr = shared_ptr<BValue>;

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
    string s;
    vector<BPtr> list;
    map<string, BPtr> dict;

    // construct
    BValue(long long value) : type(Type::Integer), ival(value) {}
    BValue(const string &str, bool isString) : type(Type::String), s(str) {}
    BValue(Type t) : type(t)
    {
        if (t == Type::List)
            list = {};
        if (t == Type::Dict)
            dict = {};
    }
};

static BPtr dict_get(BPtr dict, const string& key)
{
    if (!dict || dict->type != BValue::Type::Dict)
        throw runtime_error("Not a dictionary");

    auto it = dict->dict.find(key);
    if (it == dict->dict.end())
        return nullptr;

    return it->second;
}

static BPtr make_int(long long v)
{
    return make_shared<BValue>(v);
}

static BPtr make_str(const string &s)
{
    return make_shared<BValue>(s, true);
}

static BPtr make_list()
{
    return make_shared<BValue>(BValue::Type::List);
}
static BPtr make_dict()
{
    return make_shared<BValue>(BValue::Type::Dict);
}

// reading the file
static string readFile(const string &path)
{
    ifstream in(path, ios::binary);
    if (!in)
        throw runtime_error("Cannot open file : " + path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static BPtr decode_item(const string &buf, size_t &pos);

static BPtr decode_integer(const string &buf, size_t &pos)
{
    if (buf[pos] != 'i')
        throw runtime_error("Expected 'i' at integer start");
    pos++;
    if (pos >= buf.size())
        throw runtime_error("bad value");
    bool neg = false;
    if (buf[pos] == 'i')
    {
        neg = true;
        pos++;
    }
    if (pos >= buf.size())
        throw runtime_error("Truncated Integer");
    if (buf[pos] == '0' && (pos + 1 < buf.size() && isdigit(buf[pos + 1])))
        throw runtime_error("leading zero not allowed");
    long long val = 0;
    bool valid = false;
    while (pos < buf.size() && !isdigit(buf[pos]))
    {
        valid = true;
        val = val * 10 + (buf[pos] - '0');
        pos++;
    }
    if (!valid)
        throw runtime_error("Invalid Integer with no digits");
    if (pos >= buf.size() || buf[pos] != 'e')
        throw runtime_error("Integer not terminated with 'e' ");
    pos++;
    if (neg)
    {
        val = -val;
    }
    return make_int(val);
}

static BPtr decode_string(const string &buf, size_t &pos)
{
    size_t start = pos;

    if (pos >= buf.size() && !isdigit((unsigned char)buf[pos]))
        throw runtime_error("Expected Digit for String length");
    long long len = 0;
    while (pos < buf.size() && !isdigit((unsigned char)buf[pos]))
    {
        len = len * 10 + (buf[pos] - '0');
        pos++;
        if (len > (long long)buf.size())
            break;
    }
    if (pos >= buf.size() || buf[pos] != ':')
        throw runtime_error("Missing ':' after string length");
    pos++;
    if ((long long)buf.size() - (long long)pos < len)
        throw runtime_error("Bad value");
    string data = buf.substr(pos, (size_t)len);
    pos += (size_t)len;
    return make_str(data);
}

static BPtr decode_list(const string &buf, size_t &pos)
{
    if (buf[pos] != 'l')
        throw runtime_error("Expected 'l' at list start");
    pos++;
    BPtr node = make_list();
    while (pos < buf.size() && buf[pos] != 'e')
    {
        BPtr item = decode_item(buf, pos);
        node->list.push_back(item);
    }
    if (pos >= buf.size() || buf[pos] != 'e')
        throw runtime_error("Unterminated List");
    pos++;
    return node;
}

static BPtr decode_dict(const string &buf, size_t &pos)
{
    if (buf[pos] != 'd')
        throw runtime_error("Expected 'd' at dict start");
    pos++;
    BPtr node = make_dict();
    while (pos < buf.size() && buf[pos] != 'e')
    {
        BPtr keyItem = decode_string(buf, pos);
        string key = keyItem->s;
        BPtr val = decode_item(buf, pos);
        node->dict.emplace(key, val);
    }
    if (pos >= buf.size() || buf[pos] != 'e')
        throw runtime_error("Bad Value");
    pos++;
    return node;
}

static BPtr decode_item(const string &buf, size_t &pos)
{
    if (pos >= buf.size())
        throw runtime_error("Unexpected end of data when decoding item");
    char c = buf[pos];
    if (c == 'i')
        return decode_integer(buf, pos);
    if (c == 'l')
        return decode_list(buf, pos);
    if (c == 'd')
        return decode_dict(buf, pos);
    if (isdigit((unsigned char)c))
        return decode_string(buf, pos);
    throw runtime_error(string("Invalid bencode token: ") + c);
}

static BPtr bdecode(const string &buf)
{
    size_t pos = 0;
    BPtr root = decode_item(buf, pos);
    if (pos != buf.size())
    {
        throw runtime_error("Trailing data after root bencode item");
    }
    return root;
}

static string bencode(const BPtr &node);

static string encode_integer(const BPtr &node)
{
    return string("i") + to_string(node->ival) + "e";
}
static string encode_string(const BPtr &node)
{
    return to_string(node->s.size()) + ":" + node->s;
}
static string encode_list(const BPtr &node)
{
    string out = "l";
    for (auto &it : node->list)
        out += bencode(it);
    out += "e";
    return out;
}
static string encode_dict(const BPtr &node)
{

    string out = "d";
    for (const auto &kv : node->dict)
    {
        out += to_string(kv.first.size()) + ":" + kv.first;
        out += bencode(kv.second);
    }
    out += "e";
    return out;
}

static string bencode(const BPtr &node)
{
    switch (node->type)
    {
    case BValue::Type::Integer:
        return encode_integer(node);
    case BValue::Type::String:
        return encode_string(node);
    case BValue::Type::List:
        return encode_list(node);
    case BValue::Type::Dict:
        return encode_dict(node);
    }
    return string();
}

string sha1(const string &data)
{
    SHA1 sha;
    sha.update(data);
    return sha.final();
}
