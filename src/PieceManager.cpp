#include <bits/stdc++.h>
#include <lib/spdlog/spdlog.h>
#include <unistd.h>
#include "PieceManager.h"
#include "Block.h"
#include "utils.h"

#define B_SIZE 16382
#define MAX_PENDING_TIME 5
#define PROGRESS_BAR_WIDTH 40
#define PROGRESS_DISPLAY_INTERVAL 1

PieceManager::PieceManager(
    const TorrentFileParser& fileParser,
    const std::string& dPath,
    const int maxConnections
): fileParser(fileParser), maxConnections(maxConnections), pieceLength(fileParser.getPieceLength())
{
    missingPieces = initPieces();
    // create a destination file with specified size
    downloadedFile.open(dPath, std::ios::binary | std::ios::out);
    downloadedFile.seekp(fileParser.getFileSize() - 1);
    downloadedFile.write("",1);

    startingTime = std::time(nullptr);
    std::thread progressThread([this] { this->trackProgress(); } );
    progressThread.detach();
}

PieceManager::~PieceManager() {
    for(Piece* p: missingPieces)
        delete p;
    
    for(Piece* p: ongoingPieces)
        delete p;
    
    for(pRequest* p : pendingReqs)
        delete p;

    downloadedFile.close();

}

std::vector<Piece*> PieceManager::initPieces()
{
    std::vector<std::string> pieceHashes = fileParser.splitPieceHashes();
    totalPieces = pieceHashes.size();
    std::vector<Piece*> torrentPieces;
    missingPieces.reserve(totalPieces);

    long totalLength = fileParser.getFileSize();

    int blockCount = ceil(pieceLength / B_SIZE);
    long remLength = pieceLength;

    for(int i=0; i< totalPieces; i++)
    {
        if(i == totalPieces - 1)
        {
            remLength = totalLength % pieceLength;
            blockCount = std::max( (int) ceil(remLength /B_SIZE));
        }

            std::vector<Block* >blocks;
            blocks.reserve(blockCount);

            for(int offset = 0 ; offset < blockCount; offset++){
                Block* block = new Block;
                block->piece = i;
                block->status = missing;
                block->offset = offset * B_SIZE;
                int blockSize = B_SIZE;
                if( i == totalPieces - 1 && offset == blockCount - 1 )
                {
                    blockSize = remLength % B_SIZE;
                }
                block->length = blockSize;
                blocks.push_back(block);
            }
        auto piece = new Piece(i,blocks,pieceHashes[i]);
        torrentPieces.emplace_back(piece);
    }
    return torrentPieces;
}

bool PieceManager::isComplete() {
    lock.lock();
    bool isComplete = havePieces.size() == totalPieces;
    lock.unlock();
    return isComplete;
}

void PieceManager::addPeer(const std::string& peerId, std::string bitField)
{
    lock.lock();
    peers[peerId] = bitField;
    lock.unlock();
    std::stringstream info;
    info << "Number of connections: " <<  std::to_string(peers.size()) << "/" + std::to_string(maxConnections);
    SPDLOG_INFO("%s", info.str().c_str());
}

void PieceManager::updatePeer(const std::string& peerId, int index)
{
    lock.lock();
    if (peers.find(peerId) != peers.end())
    {
        setPiece(peers[peerId], index);
        lock.unlock();
    }else{
        lock.unlock();
        throw std::runtime_error("Connection has not been established with peer " + peerId);
    }
}

void PieceManager::removePeer(const std::string& peerId)
{
    if(isComplete())
        return;
    lock.lock();
    auto iter = peers.find(peerId);
    if (iter != peers.end())
    {
        peers.erase(iter);
        lock.unlock();
       std::stringstream info;
        info << "Number of connections: " <<
             std::to_string(peers.size()) << "/" + std::to_string(maxConnections);
        SPDLOG_INFO("%s", info.str().c_str()); 
}
}


Block* PieceManager::nextRequest(std::string peerId)
{
    //return next block that should be requested from the given peer.

    lock.lock();
    if (missingPieces.empty())
    {
        lock.unlock();
        return nullptr;
    }

    if(peers.find(peerId) == peers.end())
    {
        lock.unlock();
        return nullptr;
    }

    Block* block = expiredRequest(peerId);
    if(!block)
    {
        block = nextOngoing(peerId);
        if(!block)
            block = getRarestPiece(peerId)->nextRequest();
    }

    lock.unlock();

    return block;
}

Block* PieceManager::expiredRequest(std::string peerId)
{
    time_t currentTime = std::time(nullptr);
    for (pRequest* pending : pendingReqs)
    {
        if (hasPiece(peers[peerId], pending->block->piece))
        {
            auto diff = std::difftime(currentTime, pending->timestamp);
            if(diff >= MAX_PENDING_TIME)
            {
                pending->timestamp = currentTime;
                SPDLOG_INFO("Block %d from piece %d has expired", pending->block->offset, pending->block->piece);
                return pending->block;
            }
        }
    }
    return nullptr;
}

Block* PieceManager::nextOngoing(std::string peerId)
{
    for(Piece* piece : ongoingPieces)
    {
        if(hasPiece(peers[peerId], piece->index))
        {
            Block* block = piece->nextReq();
            if(block)
            {
                auto currentTime = std::time(nullptr);
                auto newPendingRequest = new pRequest;
                newPendingRequest->block = block;
                newPendingRequest->timestamp = std::time(nullptr);
                pendingReqs.push_back(newPendingRequest);
                return block;
            }
        }
    }
    return nullptr;
}

Piece* PieceManager::getRarestPiece(std::string peerId)
{
    
    auto comp = [](const Piece* a, const Piece* b) { return a->index < b->index; };
    std::map<Piece*, int, decltype(comp)> pieceCount(comp);
    for (Piece* piece : missingPieces)
    {
       
        if (peers.find(peerId) != peers.end())
        {
            if (hasPiece(peers[peerId], piece->index))
                pieceCount[piece] += 1;
        }
    }

    Piece* rarest;
    int leastCount = INT16_MAX;
    for (auto const& [piece, count] : pieceCount)
    {
        if (count < leastCount)
        {
            leastCount = count;
            rarest = piece;
        }
    }

    missingPieces.erase(
            std::remove(missingPieces.begin(), missingPieces.end(), rarest),
            missingPieces.end()
    );
    ongoingPieces.push_back(rarest);
    return rarest;
}

void PieceManager::write(Piece* piece)
{
    long position = piece->index * fileParser.getPieceLength();
    downloadedFile.seekp(position);
    downloadedFile << piece->getData();
}