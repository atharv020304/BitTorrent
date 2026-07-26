#ifndef TORRENT_PEER_RETRIVER_H
#define TORRENT_PEER_RETRIVER_H

#include <string>
#include <vector>
#include <cpr/cpr.h>


struct Peer
{
    std::string ip;
    int port;
};

class PeerRetriever
{
    private:    
        std::string announceUrl;
        std::string infoHash;
        std::string peerId;

        int port;
        const unsigned long fileSize;
        std::vector<Peer*> decodeResp(std::string response);
    public:
        explicit PeerRetriever(std::string peerId, std::string announceUrl, std::string infoHash, int port, unsigned long fileSize);
        std::vector<Peer*> retrievePeers(unsigned long bytesDnld = 0);
};

#endif /*TORRENT_PEER_RETRIVER_H*/
