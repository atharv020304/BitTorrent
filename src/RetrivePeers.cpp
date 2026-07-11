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


//constructor
PeerRetriever::PeerRetriever(
        std::string peerId,
        std::string announceUrl,
        std::string infoHash,
        int port,
        const unsigned long fileSize
){
    this->peerId = std::move(peerId);
    this->announceUrl = std::move(announceUrl);
    this->infoHash = std::move(infoHash);
    this->port = port;
    this->fileSize = fileSize;
}

std::vector<Peer* > PeerRetriever::retrivePeers(unsigned long bytesDnld)
{
    std::stringstream info;
    info << "Retrieving peers from " << announceUrl << "with following params... " << std::endl;
    info << "peer_id: "<< peerId << std::endl;
    info << "port: " << port << std::endl;
    info << "uploaded: " << 0 << std::endl;
    info << "downloaded: " << std::to_string(bytesDnld) << std::endl;
    info << "left :" << std::to_string(fileSize - bytesDnld) << std::endl;
    info << "compact: " << std::to_string(1);
    
    SPDLOG_INFO("%s", info.str().c_str());

    cpr::Response res = cpr::Get(cpr::Url{announceUrl}, cpr::Parameters{
        { "info_hash", std::string(hexDecode(infoHash))},
        { "peer_id", std::string(peerId) },
        { "port", std::to_string(port) },
        { "uploaded", std::to_string(0) },
        { "downloaded", std::to_string(bytesDnld)},
        { "left", std::to_string(fileSize - bytesDnld)},
        { "compact", std::to_string(1) }
    }, cpr::Timeout{ TRACKER_TIMEOUT });

    if(res.status_code == 200)
    {
        SPDLOG_INFO("Retrieve response from tracker: SUCCESS");
        std::vector<Peer*> peers = decodeResp(res.text);
        return peers;

    }else
    {
        SPDLOG_INFO("ERROR response from tracker")
    }
    return std::vector<Peer*> ();
}


std::vector<Peer*> PeerRetriever::decodeResp(std::string response) {
    SPDLOG_INFO("Decoding tracker response");
    std::shared_ptr<bencoding::BItem> dResp = bencoding::decode(response);

    std::shared_ptr<bencoding::BDictionary> responseDict = 
        std::dynamic_pointer_cast<bencoding::BDictionary>(dResp);
    
    std::shared_ptr<bencoding::BItem> peersValue = responseDict->getValue("peers");
    if(!peersValue)

    std::vector<Peer*> peers;

    // case 1 : peer info is in binaru blob
    if(typeid(*peersValue) == typeid(bencoding::BString))
    {
        const int peerInfoSize = 6;
        std::string peerString = std::dynamic_pointer_cast<bencoding::BString>(peersValue)->value();

        if(peerString.length() % peerInfoSize != 0)
        {
            throw::std::runtime_error("Recieved malformed peers form tracker");
        }

        const int peerNum = peersString.length() / peerInfoSize;
        for(int i=0; i< peerNum; i++)
        {
            int offset = i * peerInfoSize;
            std::stringstream peerIp;
            peerIp << std::to_string((uint8_t) peersString[offset]) << ".";
            peerIp << std::to_string((uint8_t) peersString[offset + 1]) << ".";
            peerIp << std::to_string((uint8_t) peersString[offset + 2]) << ".";
            peerIp << std::to_string((uint8_t) peersString[offset + 3]);
            int peerPort = bytesToInt(peerString.substr(offset+4, 2));

            Peer* newPeer = new Peer { peerIp.str() , peerPort};
            peers.push_back(newPeer);
        }
    }
    // case 2 : peer info is in List format
    else if (typeid(*peersValue) == typeid(bencoding::BList))
    {
        std::shared_ptr<bencoding::BList> peerList = std::dynamic_pointer_cast<bencoding::BList>(peersValue);
        for(auto &item : *peerList)
        {
            //cast the item to a dict
            std::shared_ptr<bencoding::BDictionary> peerDict = std::dynamic_pointer_cast<bencoding::BDictionary>(item);

            //gets peer ip from the dictionary
            std::shared_ptr<bencoding::BItem> tempPeerIp = peerDict->getValue("ip");

            if(!tempPeerIp){
                throw std::runtime_error("Received malformed 'peers' from tracker. [Item does not contain key 'ip']");
            }
            std::string peerIp = std::dynamic_pointer_cast<bencoding::BString>(tempPeerIp)->value();
            std::shared_ptr<bencoding::BItem> tempPeerPort = peerDict->getValue("port");
            if (!tempPeerPort)
                throw std::runtime_error("Received malformed 'peers' from tracker. [Item does not contain key 'port']");
            
            int peerPort = (int) std::dynamic_pointer_cast<bencoding::BInteger>(tempPeerPort)->value();
            Peer* newPeer = new Peer { peerIp, peerPort };
            peers.push_back(newPeer);
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