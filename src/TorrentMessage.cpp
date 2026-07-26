#include <iostream>
#include <sstream>
#include <bitset>
#include <arpa/inet.h>
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
    uint32_t networkLength = htonl(length);
    char* mLengthAddr = (char*)&networkLength; 
    std::string mLengthStr;
    
    for(int i = 3; i >= 0 ;i--)
    {
        mLengthStr.push_back((char) mLengthAddr[i]);
    }
    buff << mLengthStr;
    buff << (char) id;
    buff << payload;
    return buff.str();

}
