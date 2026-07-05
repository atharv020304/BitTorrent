#ifndef TORRENTMESSAGE_H
#define TORRENTMESSAGE_H

#include <cstdint>

enum messageID
{
    choke = 0,
    unchoke = 1,
    interested = 2,
    notInterested = 3,
    have = 4,
    bitField = 5,
    request = 6,
    piece = 7,
    cancel = 8
};

class TorrentMessage
{
    private:
        uint32_t length;
        uint8_t id;
        std::string payload;
    
    public:
        explicit TorrentMessage(uint8_t id, const std::string& payload = "");
        std::string toString();
        uint8_t getMessageId();
        std::string getPayload();

};

#endif~