
#include <iomanip>
#include <iostream>
#include <cmath>
#include <string>
#include <bitset>


std::string urlEncode(const std::string &value)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex; // change the stream to hex

    for(char c : value)
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char) c);
        escaped << std::nouppercase;
    }
    return escaped.str();
}

std::string hexDecode(const std::string& value)
{
    int hashLength = value.length();
    std::string decodedString;
    for(int i = 0 ; i < hashLength; i+=2){
        std::string byte = value.substr(i, 2);
        char c = (char) (int) strtol(byte.c_str(), nullptr, 16);
        decodedString.push_back(c);
    }
    return decodedString;
}

std::string hexEncode(const std::string& input)
{
    static const char hexDigits[] = "0123456789ABCDEF";
    std::string output;
    for(unsigned char c : input)
    {
        output.push_back('\\');
        output.push_back('x');
        output.push_back(hexDigits[c >> 4]);
        output.push_back(hexDigits[c & 15]);

    }
    return output;
}

bool hasPiece(const std::string& bitField, int index){
    if (index < 0 || (size_t)index / 8 >= bitField.size()) return false;
    return ((unsigned char)bitField[index / 8] >> (7 - index % 8)) & 1; 
}

void setPiece(std::string& bitField, int index)
{
    if (index < 0 || (size_t)index / 8 >= bitField.size()) return;
    bitField[index / 8] = (char)((unsigned char)bitField[index / 8] | (1u << (7 - index % 8)));
}


int bytesToInt(std::string bytes)
{
    uint32_t v = 0;
    for (unsigned char c : bytes) v = (v << 8) | c;
    return (int)v;
}

// using as it is from gfg
std::string formatTime(long seconds)
{
    if (seconds < 0)
        return "inf";

    std::string result;
    // compute h, m, s
    std::string h = std::to_string(seconds / 3600);
    std::string m = std::to_string((seconds % 3600) / 60);
    std::string s = std::to_string(seconds % 60);
    // add leading zero if needed
    std::string hh = std::string(2 - h.length(), '0') + h;
    std::string mm = std::string(2 - m.length(), '0') + m;
    std::string ss = std::string(2 - s.length(), '0') + s;
    // return mm:ss if hh is 00
    if (hh.compare("00") != 0) {
        result = hh + ':' + mm + ":" + ss;
    }
    else {
        result =  mm + ":" + ss;
    }
    return result;
}