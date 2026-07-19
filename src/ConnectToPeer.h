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
        SharedQueue<Peer*>* queue;
        Peer* peer;
        std::string peerBitField;
        std::string peerId;
        PieceManager* pieceManager;

        std::string createHandShakeMessage();
        void performHandShake();
        void recieveBitField();
        void sendInterested();
        void receiveUnchoke();
        void requestPiece();
        void closeSock();
        bool establisshNewConnection();
        TorrentMessage receiveMessage(int bufferSize = 0);
    
    public:
        const std::string &getPeerId() const;
        explicit PeerConnection(SharedQueue<Peer*>* queue,std::string clientId, std::string infoHash, PieceManager* pieceManager);
        ~PeerConnection();
        void start();
        void stop();
};


#endif /*TORRENT_PEERCONNECT_H*/

