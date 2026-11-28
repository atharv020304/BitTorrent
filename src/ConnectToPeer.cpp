#include <stdexcept>
#include <iostream>
#include <cassert>
#include <cstring>
#include <chrono>
#include <unistd.h>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <utility>

#include "ConnectToPeer.h"
#include "utils.h"
#include "connect.h"

#define INFO_HASH_STARTING_POS 28
#define PEER_ID_STARTING_POS 48
#define HASH_LEN 20
#define DUMMY_PEER_IP "0.0.0.0"

static std::string makeConnectionKey() { std::atomic<int> n{0};  return "conn-"+std::to_string(++n);} 


PeerConnection::PeerConnection(
    SharedQueue<Peer>* queue,
    std::string clientId,
    std::string infoHash,
    PieceManager* pieceManager
) : clientId(std::move(clientId)), infoHash(infoHash),queue(queue), connKey(makeConnectionKey()), pieceManager(pieceManager) {}

void PeerConnection::stop()
{
    terminated = true;
}

void PeerConnection::runSession()
{
    using clock = std::chrono::steady_clock;
    auto lastActivity = clock::now(), lastKeepAlive = clock::now();
    while(!terminated && !pieceManager->isComplete())
    {
        try
        {
            TorrentMessage message = receiveMessage();
            lastActivity = clock::now();
            handleMessage(message);
        }
        catch (const ReadTimeout&)
        {
            auto now = clock::now();
            if ( now - lastActivity > std::chrono::seconds(150)) throw std::runtime_error("peer silent for too long");
            if ( now - lastKeepAlive > std::chrono::seconds(60)) { sendData(sock, std::string(4, '\0')); lastKeepAlive = now; }
        }
        if(requestPending && clock::now() - lastRequestTime > std::chrono::seconds(30))
            throw std::runtime_error("peer did not answer a block request in time");
        if(!choked && !requestPending) requestPiece();
    }
}

void PeerConnection::handleMessage(TorrentMessage& message)
{
    switch (message.getMessageId())
    {
        case  keepAlive: break;
        case  choke:     choked = true; requestPending = false; break;
        case unchoke:    choked = false; break;
        case bitField:   
                pieceManager->addPeer(connKey, message.getPayload());
                registerd = true;
                break;
        case have:
        {
                std::string p = message.getPayload();
                if (p.size() != 4) throw std::runtime_error("malformed 'have'");
                pieceManager->updatePeer(connKey, bytesToInt(p));
                break;
        }  
        case piece:
        {
                std::string p = message.getPayload();
                if (p.size() < 8) throw std::runtime_error("malformed 'piece");
                requestPending = false;
                pieceManager->blockReceived( connKey, bytesToInt(p.substr(0, 4)), bytesToInt(p.substr(4,4)), p.substr(8));
                break;
        }
        default : break;
    }
}

void PeerConnection::start()
{
    while(!(terminated || pieceManager->isComplete()))
    {
        Peer next = queue->pop_front();
        if (next.ip == DUMMY_PEER_IP) return;
        peer = next;
        try { if(establishNewConnection()) runSession(); }
        catch (const std::exception& e) { SPDLOG_INFO("Connection to peer [{}:{}] ended: {}", peer.ip, peer.port,e.what()); }
        closeSock();
    }
}

void PeerConnection::performHandshake()
{
    sock  = createConnection(peer.ip, peer.port);
    SPDLOG_INFO("Established TCP connection with peer at socket %d: successfully", sock);

    SPDLOG_INFO("Sending Handshake msg to %s ...", peer.ip.c_str());
    std::string hsMsg = createHandshakeMessage();
    sendData(sock, hsMsg);
    SPDLOG_INFO("Sending handshake msg success");

    SPDLOG_INFO("Receiving the handshake reply");
    std::string reply = receiveData(sock, hsMsg.length());
    if (reply.empty())
        throw std::runtime_error("Receive handshake from peer: FAILED [No response from peer]");
    peerId = reply.substr(PEER_ID_STARTING_POS,HASH_LEN);
    SPDLOG_INFO("Receive handshake reply from peer successfully");

    std::string receivedInfoHash = reply.substr(INFO_HASH_STARTING_POS, HASH_LEN);
    if(receivedInfoHash != hexDecode(infoHash))
        throw std::runtime_error("handshake failed");
    SPDLOG_INFO("hash Comparison : Success");

}

void PeerConnection::sendInterested()
{
    SPDLOG_INFO("Sending Interested message to peer [%s]...", peer.ip.c_str());
    std::string interestedMessage = TorrentMessage(interested).toString();
    sendData(sock, interestedMessage);
    SPDLOG_INFO("Send Interested message: SUCCESS");
}

void PeerConnection::requestPiece() 
{
    Block* block = pieceManager->nextRequest(peerId);

    if(!block)
        return;
    
    int payloadLength = 12;
    char temp[payloadLength];

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
    info << "Sending Request message to peer " << peer.ip << " ";
    info << "[Piece: " << std::to_string(block->piece) << " ";
    info << "Offset: " << std::to_string(block->offset) << " ";
    info << "Length: " << std::to_string(block->length) << "]";
    SPDLOG_INFO("%s", info.str().c_str());
    std::string reqMsg = TorrentMessage(request, payload).toString();
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
    catch( const std::runtime_error& e)
    {
        SPDLOG_INFO("An error occured while connecting with peer [%s]", peer.ip.c_str());
        return false;
    }
}

std::string PeerConnection::createHandshakeMessage()
{
    const std::string protocol = "BitTorrent protocol";
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

TorrentMessage PeerConnection::receiveMessage(int bufferSize){
    std::string reply = receiveData(sock, bufferSize);
    if(reply.empty())
        return TorrentMessage(keepAlive);
    auto messageId = (uint8_t) reply[0];
    std::string payload = reply.substr(1);
    SPDLOG_INFO("Received message with ID %d from peer [%s]", messageId, peer.ip.c_str());
    return TorrentMessage(messageId, payload);
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
        sock = 0;
        requestPending = false;
        if(!peerBitField.empty())
        {
            peerBitField.clear();
            pieceManager->removePeer(peerId);
        }
    }
}
