#include <stdexcept>
#include <iostream>
#include <cassert>
#include <cstring>
#include <chrono>
#include <unistd.h>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <utility>

#include "PeerConnection.h"
#include "utils.h"
#include "connect.h"

#define INFO_HASH_STARTING_POS 28
#define PEER_ID_STARTING_POS 48
#define HASH_LEN 20
#define DUMMY_PEER_IP "0.0.0.0"

PeerConnection::PeerConnection(
    SharedQueue<Peer*>* queue,
    std::string clientId,
    std::string infoHash,
    PieceManager* pieceManager
) : queue(queue), clientId(std::move(clientId)), infoHash(infoHash), pieceManager(pieceManager) {}

void PeerConnection::stop()
{
    terminated = true;
}

void PeerConnection::start()
{
    SPDLOG_INFO("Downloading thread started...");
    while(!(terminated || pieceManager->isComplete()))
    {
        p = queue->pop_front();

        if(p->ip == DUMMY_PEER_IP)
            return;
        
        try
        {
            if(establishNewConnection()){
                while(!pieceManager->isComplete())
                {
                    BitTorrentMessage message = receiveMessage();
                    if (message.getMessageId() > 10)
                        throw std::runtime_error("invalid msg");
                    switch(message.getMessageId())
                    {
                        case choke:
                            chocked = true;
                            break;

                        case unchoke:
                            chocked = false;
                            break;
                        
                        case piece:
                        {
                            requestPending = false;
                            std::string payload = message.getPayload();
                            int index = bytesToInt(payload.substr(0,4));
                            int begin = bytesToInt(payload.substr(4,4));
                            std::string blockData = payload.substr(8);
                            pieceManager->blockReceived(peerId, index, begin, blockData);
                            break;
                        }
                        case have:
                        {
                            std::string payload = message.getPayload();
                            int pieceIndex = bytesToInt(payload);
                            pieceManager->updatePeer(peerId, pieceIndex);
                            break;
                        }

                        default:
                            break;
                    }

                    if(!choked)
                    {
                        if(!requestPending)
                        {
                            requestPiece();
                        }
                    }
                }
            }
        }
        catch(const std::exception& e)
        {   
            closeSock();
            std::cerr << e.what() << '\n';
        }
        
    }
}

void PeerConnection::performHandshake()
{
    try
    {
        sock  = createConnection(peer->ip, peer->port);
    }
    catch (std::runtime_error &e)
    {
        throw std::runtime_error("Cannot connect to a peer");
    }
    SPDLOG_INFO("Established TCP connection with peer at socket %d: successfully", sock);

    // first handshake msg to peer
    SPDLOG_INFO("Sending Handshake msg to %s ...", peer->ip);
    std::string hsMsg = createHandshakeMessage();
    sendData(sock, hsMsg);
    SPDLOG_INFO("Sending handshake msg success");

    SPDLOG_INFO("Receiveing the handshake reply");
    std::string reply = recieveData(sock, hsMsg.length());
    if (reply.empty())
        throw std::runtime_error("Receive handshake from peer: FAILED [No response from peer]");
    peerId = reply.substr(PEER_ID_STARTING_POS,HASH_LEN);
    SPDLOG_INFO("Receive handshake reply from peer successfully");

    std::string receivedInfoHash = reply.substr(INFO_HASH_STARTING_POS, HASH_LEN);
    if((receivedInfoHash == infoHash) != 0)
        throw std::runtime_error("handshake failed");
    SPDLOG_INFO("hash Comparison : Success");

}

void PeerConnection::receiveBitField()
{
    SPDLOG_INFO("Receiving BitField message from peer %s", peer->ip);
    BitTorrentMessage msg = receiveMessage();
    if(msg.getMessageId() != bitField);
        throw std::runtime_error("failed to receive the correct bitfield");
    peerBitField = msg.getPayload();

    pieceManager->addPeer(peerId, peerBitField);

    SPDLOG_INFO("Receive BitField Successfull");
}

