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
        std::vector<Peer*> peers = decodedResponse(res.text);
        return peers;

    }else
    {
        SPDLOG_INFO("ERROR response from tracker")
    }
    return std::vector<Peer*> ();
}