/**
 * @file ConnectToPeer.h
 * @brief BitTorrent peer wire protocol handling.
 */

#ifndef TORRENT_PEERCONNECT_H
#define TORRENT_PEERCONNECT_H

#include "RetrivePeers.h"
#include "TorrentMessage.h"
#include "PieceManager.h"
#include "SharedQueue.h"


/**
 * @class PeerConnection
 * @brief Worker that connects to one peer and exchanges BitTorrent messages.
 *
 * Performs the handshake, receives the bitfield, sends interested/request messages,
 * and forwards downloaded blocks to @ref PieceManager.
 */
class PeerConnection
{
    private:    
        int sock{};
        bool choked = true;
        bool requestPending = false;
        bool terminated = false;
        const std::string clientId;
        const std::string infoHash;
        SharedQueue<Peer>* queue;
        Peer peer;
        std::string peerBitField;
        std::string peerId;
        PieceManager* pieceManager;

        std::string createHandshakeMessage();
        void performHandshake();
        void receiveBitField();
        void sendInterested();
        void receiveUnchoke();
        void requestPiece();
        void closeSock();
        bool establishNewConnection();
        TorrentMessage receiveMessage(int bufferSize = 0);
    
    public:
        const std::string &getPeerId() const;
        explicit PeerConnection(SharedQueue<Peer>* queue, std::string clientId, std::string infoHash, PieceManager* pieceManager);
        void start();
        void stop();
};


#endif /*TORRENT_PEERCONNECT_H*/
