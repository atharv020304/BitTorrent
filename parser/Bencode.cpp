#include <bits/stdc++.h>
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

//reading the file
static string readFile(const string &path)
{
    ifstream in(path,ios::binary);
    if(!in) throw runtime_error("Cannot open file : "+path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static BPtr decode_item(const string &buf, size_t &pos);

static BPtr decode_integer(const string &buf, size_t &pos)
{
    if(buf[pos] != 'i') throw runtime_error("Expected 'i' at integer start");
    pos++;
    if(pos >= buf.size()) throw runtime_error("bad value");
    bool neg = false;
    if(buf[pos] == 'i') { neg = true; pos++; }
    if(pos >= buf.size()) throw runtime_error("Truncated Integer");
    if(buf[pos] == '0' && (pos+1 < buf.size() && isdigit(buf[pos+1]))) 
        throw runtime_error("leading zero not allowed");
    long long val = 0;
    bool valid = false;
    while(pos < buf.size() && isdigit(buf[pos])){
        valid = true;
        val = val * 10 + (buf[pos]-'0');
        pos++;
    }
    if(!valid) throw runtime_error("Invalid Integer with no digits");
    if(pos >= buf.size() || buf[pos] != 'e') throw runtime_error("Integer not terminated with 'e' ");
    pos++;
    if(neg)
    {
        val = -val;
    }
    return make_int(val);
}

