#include <iostream>
#include <sstream>
#include <bitset>
#include "TorrentMessage.h"


TorrentMessage::TorrentMessage(const uint8_t id, const std::string &payload):
        id(id),
        payload(payload),
        length(payload.length() + 1) {}


uint8_t TorrentMessage::getMessageId() 
{
    return id;
}

std::string TorrentMessage::getPayload()
{
    return payload;
}

std::string TorrentMessage::toString()
{
    std::stringstream buff;
    char* mLengthAddr = (char*)& length; 
    std::string mLengthStr;
    
    // Access length as bytes and serialize in network byte order
    for(int i = 4; i >= 0 ;i--)
    {
        mLengthStr.push_back((char) mLengthAddr[i]);
    }
    buff << mLengthStr;
    buff << (char) id;
    buff << payload;
    return buff.str();

}