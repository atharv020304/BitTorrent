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
    std::string out;
    out.push_back(char(length >> 24));
    out.push_back(char(length >> 16));
    out.push_back(char(length >> 8));
    out.push_back(char(length));
    out.push_back(char(id));
    out += payload;
    return out
}
