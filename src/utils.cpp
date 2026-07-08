
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
    int byteIndex = floor(index / 8);
    int offset = index % 8;
    return (bitField[byteIndex] >> (7 - offset)& 1) != 0;
}

bool setPiece(std::string& bitField, int index)
{
    int byteIndex = floor(index / 8);
    int ofst = index % 8;
    bitField[byteIndex] |= ( 1 << ( 7 - ofst));  //mark a peice as downloaded by setting the bit as 1
}


int bytesToInt(std::string bytes)
{
    std::string binStr;
    long n = bytes.size();
    for(int i = 0 ; i < n; i++)
    {
        binStr += std::bitset<8>(bytes[i]).to_string();  //storing as bit
    }
    return stoi(binStr, 0, 2); //convert to integer base 2.
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