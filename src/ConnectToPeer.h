#ifndef TORRENT_PEERCONNECT_H
#define TORRENT_PEERCONNECT_H

#include "RetrivePeers.h"
#include "TorrentMessage.h"
#include "PieceManager.h"
#include "SharedQueue.h"


using byte = unsigned char;

class PeerConnection
{
    private:    
        int sock{};
        bool choked =  true;
        bool reqPending = false;
        bool terminated = false;
        const std::string clientId;
        const std::string infoHash;
}


#endif /*TORRENT_PEERCONNECT_H*/

