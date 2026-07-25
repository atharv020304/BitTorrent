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

void PeerConnection::sendInterested()
{
    SPDLOG_INFO("Sending Interested message to peer [%s]...", peer);
    std::string interestedMessage = BitTorrentMessage(interested).toString();
    sendData(sock, interestedMessage);
    SPDLOG_INFO(INFO, "Send Interested message: SUCCESS");
}

void PeerConnection::receiveUnchoke() {
    SPDLOG_INFO("Receiving Unchoke message from peer [%s]...", peer->ip);
    BitTorrentMessage message = receiveMessage();
    if (message.getMessageId() != unchoke)
        throw std::runtime_error(
                "Receive Unchoke message from peer: FAILED [Wrong message ID: " +
                std::to_string(message.getMessageId()) + "]");
    choked = false;
    SPDLOG_INFO("Receive Unchoke message: SUCCESS");
}

void PeerConnection::requestPiece() 
{
    Block* block = pieceManager->nextRequest(peerId);

    if(!block)
        return;
    
    int payloadLength = 12;
    char temp[payloadLength];

    //little indian to big indian conversion
    uint32_t index = htonl(block->piece);
    uint32_t offset = htonl(block->offset);
    uint32_t length = htonl(block->length);
    std::memcpy(temp,&index,sizeof(int));
    std::memcpy(temp+4, &offset, sizeof(int));
    std::memcpy(temp+8, &length, sizeof(int));
    std::string payload;
    for (int i=0;i < payloadLength; i++)
    {
        payload += (char) temp[i];
    }
    std::stringstream info;
    info << "Sending Request message to peer " << peer->ip << " ";
    info << "[Piece: " << std::to_string(block->piece) << " ";
    info << "Offset: " << std::to_string(block->offset) << " ";
    info << "Length: " << std::to_string(block->length) << "]";
    SPDLOG_INFO("%s", info.str());
    std::string reqMsg = BitTorrentMessage(request, payload).toString();
    sendData(sock, reqMsg);
    requestPending = true;
    SPDLOG_INFO("Send req success");

}

bool PeerConnection::establishNewConnection()
{
    try{
        performHandshake();
        receiveBitField();
        sendInterested();
        return true;
    }
    catch( const std::runtime_error* e)
    {
        SPDLOG_INFO("An error occured while connecting with peer [%s]", peer->ip);
        return false;
    }
}

std::string PeerConnection::createHandshakeMessage()
{
    const std::string protocol = "BitTorrent";
    std::stringstream buffer;
    buffer << (char) protocol.length();
    buffer << protocol;
    std::string reserved;
    for (int i=0;i< 8; i++)
    {
        reserved.push_back('\0');
    }
    buffer << reserved;
    buffer << hexDecode(infoHash);
    buffer << clientId;
    assert (buffer.str().length() == protocol.length() + 49);
    return buffer.str();
}

BitTorrentMessage PeerConnection::receiveMessage(int bufferSize) const{
    std::string reply = receiveData(sock, 0);
    if(reply.empty())
        return BitTorrentMessage(keepAlive);
    auto messageId = (uint8_t) reply[0];
    std::string payload = reply.substr(1);
    SPDLOG_INFO("Received message with ID %d from peer [%s]", messageId, peer->ip)
    return BitTorrentMessage(messageId, payload);
}

const std::string &PeerConnection::getPeerId() const {
    return peerId;
}

void PeerConnection::closeSock()
{
    if(sock)
    {
        SPDLOG_INFO("Closed connection at socket %d", sock);
        close(sock);
        sock ={};
        requestPending = false;
        if(!peerBitField.empty())
        {
            peerBitField.clear();
            pieceManager->removePeer(peerId);
        }
    }
}