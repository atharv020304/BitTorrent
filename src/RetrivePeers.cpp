#include <sstream>
#include <string>
#include <iostream>
#include <cpr/cpr.h>
#include <random>
#include <stdexcept>
#include <bitset>
#include <bencode/bencoding.h>
#include <spdlog/spdlog.h>
#include <utility>

#include "utils.h"
#include "RetrivePeers.h"

#define TRACKER_TIMEOUT 15000


PeerRetriever::PeerRetriever(
        std::string peerId,
        std::string announceUrl,
        std::string infoHash,
        int port,
        const unsigned long fileSize
) : peerId(std::move(peerId)),
    announceUrl(std::move(announceUrl)),
    infoHash(std::move(infoHash)),
    port(port),
    fileSize(fileSize)
{
}

std::vector<Peer> PeerRetriever::retrievePeers(unsigned long bytesDnld)
{
    const unsigned long left = fileSize > bytesDnld ? fileSize - bytesDnld : 0;
    SPDLOG_INFO("Retrieving peers from {} (downloaded {}, left{})", announceUrl, bytesDnld, left);

    cpr::Parameters params{
        { "info_hash", std::string(hexDecode(infoHash)) }, {"peer_id", peerId}, {"port", std::to_string(port) },
        { "uploaded", "0" }, { "downloaded", std::to_string(bytesDnld) }, { "left", std::to_string(left) }, {"compact", "1"} 
    };

    if(!event.empty()) params.Add(cpr::Parameter{ "event", event });

    cpr::Response res = cpr::Get(cpr::Url{announceUrl}, params, cpr::Timeout{ TRACKER_TIMEOUT });
    if(res.error)       throw std::runtime_error("Tracker request failed: " + res.error.message);
    if(res.status_code != 200) throw std::runtime_error("Tracker responded with HTTP status "+ std::to_string(res.status_code));
    return decodeResp(res.text);
}


std::vector<Peer> PeerRetriever::decodeResp(std::string response) {
    SPDLOG_INFO("Decoding tracker response");
    std::shared_ptr<bencoding::BItem> dResp = bencoding::decode(response);

    std::shared_ptr<bencoding::BDictionary> responseDict = 
        std::dynamic_pointer_cast<bencoding::BDictionary>(dResp);
    
    std::shared_ptr<bencoding::BItem> peersValue = responseDict->getValue("peers");
    if(!peersValue)
        throw std::runtime_error("Received malformed 'peers' from tracker. [Missing 'peers' key]");

    std::vector<Peer> peers;

    // case 1 : peer info is in binary blob
    if(typeid(*peersValue) == typeid(bencoding::BString))
    {
        const int peerInfoSize = 6;
        std::string peerString = std::dynamic_pointer_cast<bencoding::BString>(peersValue)->value();

        if(peerString.length() % peerInfoSize != 0)
        {
            throw std::runtime_error("Received malformed peers from tracker");
        }

        const int peerNum = peerString.length() / peerInfoSize;
        for(int i=0; i< peerNum; i++)
        {
            int offset = i * peerInfoSize;
            std::stringstream peerIp;
            peerIp << std::to_string((uint8_t) peerString[offset]) << ".";
            peerIp << std::to_string((uint8_t) peerString[offset + 1]) << ".";
            peerIp << std::to_string((uint8_t) peerString[offset + 2]) << ".";
            peerIp << std::to_string((uint8_t) peerString[offset + 3]);
            int peerPort = bytesToInt(peerString.substr(offset+4, 2));

            peers.push_back(Peer { peerIp.str() , peerPort});
        }
    }
    // case 2 : peer info is in List format
    else if (typeid(*peersValue) == typeid(bencoding::BList))
    {
        std::shared_ptr<bencoding::BList> peerList = std::dynamic_pointer_cast<bencoding::BList>(peersValue);
        for(auto &item : *peerList)
        {
            std::shared_ptr<bencoding::BDictionary> peerDict = std::dynamic_pointer_cast<bencoding::BDictionary>(item);

            std::shared_ptr<bencoding::BItem> tempPeerIp = peerDict->getValue("ip");

            if(!tempPeerIp){
                throw std::runtime_error("Received malformed 'peers' from tracker. [Item does not contain key 'ip']");
            }
            std::string peerIp = std::dynamic_pointer_cast<bencoding::BString>(tempPeerIp)->value();
            std::shared_ptr<bencoding::BItem> tempPeerPort = peerDict->getValue("port");
            if (!tempPeerPort)
                throw std::runtime_error("Received malformed 'peers' from tracker. [Item does not contain key 'port']");
            
            int peerPort = (int) std::dynamic_pointer_cast<bencoding::BInteger>(tempPeerPort)->value();
            peers.push_back(Peer { peerIp, peerPort });
        }
    }
    else
    {
        throw std::runtime_error(
            "Response returned by the tracker is not in the correct format. ['peers' has the wrong type]"
        );
    }
    SPDLOG_INFO("Decoding tracker response successfull");
    SPDLOG_INFO("Number of peers discovered: %zu", peers.size());
    return peers;
}
