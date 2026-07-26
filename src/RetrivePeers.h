/**
 * @file RetrivePeers.h
 * @brief Tracker communication and peer discovery.
 */

#ifndef TORRENT_PEER_RETRIVER_H
#define TORRENT_PEER_RETRIVER_H

#include <string>
#include <vector>
#include <cpr/cpr.h>


/**
 * @struct Peer
 * @brief Endpoint returned by a BitTorrent tracker.
 */
struct Peer
{
    std::string ip;
    int port;
};

/**
 * @class PeerRetriever
 * @brief Queries the announce URL and decodes compact or dictionary peer lists.
 */
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
