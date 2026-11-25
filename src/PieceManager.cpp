#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <climits>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <cmath>

#include "PieceManager.h"
#include "Block.h"
#include "utils.h"

#define B_SIZE 16384
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
    downloadedFile.open(dPath, std::ios::binary | std::ios::out | std::ios::trunc);
    if(!downloadedFile.is_open())
    {
        for (Piece* p : missingPieces) delete p;
        throw std::runtime_error("Can not open file");
    }
    downloadedFile.seekp(fileParser.getFileSize() - 1);
    downloadedFile.put('\0');
    downloadedFile.flush();
    startingTime = std::time(nullptr);
    progressThread = std::thread([this] { trackProgress(); });
}

PieceManager::~PieceManager() {
    { std::lock_guard<std::mutex> g(progressMutex); stopProgress = true; }
    progressCv.notify_all();
    if(progressThread.joinable()) progressThread.join();
    for(Piece* p: missingPieces)
        delete p;
    
    for(Piece* p: ongoingPieces)
        delete p;
    
    for(pRequest* p : pendingReqs)
        delete p;

    for(Piece* p : havePieces)
    {
        delete p;
    }

    downloadedFile.close();

}

std::vector<Piece*> PieceManager::initPieces()
{
    std::vector<std::string> pieceHashes = fileParser.splitPieceHashes();
    totalPieces = pieceHashes.size();
    std::vector<Piece*> torrentPieces;
    torrentPieces.reserve(totalPieces);

    long totalLength = fileParser.getFileSize();

    for(int i=0; i< totalPieces; i++)
    {
        long thisPieceLength = pieceLength;
        if (i ==totalPieces - 1)
            thisPieceLength = totalLength - (long)(totalPieces - 1) * pieceLength;

        const int blockCount = (int)((thisPieceLength + B_SIZE - 1) / B_SIZE);
        std::vector<Block*> blocks;
        blocks.reserve(blockCount);
        for(int b=0; b <blockCount; b++)
        {
            Block* block = new Block;
            block->piece = i;
            block->status = missing;
            block->offset = b * B_SIZE;
            block->length = (int)std::min<long>(B_SIZE, thisPieceLength - (long)b * B_SIZE);
            blocks.push_back(block);
        }
        torrentPieces.emplace_back(new Piece(i, blocks, pieceHashes[i]));
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
        std::stringstream info;
        info << "Number of connections: " <<
             std::to_string(peers.size()) << "/" + std::to_string(maxConnections);
        SPDLOG_INFO("%s", info.str().c_str());
    }
    lock.unlock();
}


Block* PieceManager::nextRequest(std::string peerId)
{
    //return next block that should be requested from the given peer.

    lock.lock();
    if (missingPieces.empty() && ongoingPieces.empty())
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
        {
            Piece* piece = getRarestPiece(peerId);
            if (piece)
                {
                    block = piece->nextReq();
                    if(block)
                    {
                        auto* req = new pRequest;
                        req->block = block;
                        req->timestamp = std:time(nullptr);
                        pendingReqs.push_back(req);
                    }
                }
        }
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
            Block* block = piece->nextRequest();
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

    Piece* rarest = nullptr;
    int leastCount = INT16_MAX;
    for (auto const& [piece, count] : pieceCount)
    {
        if (count < leastCount)
        {
            leastCount = count;
            rarest = piece;
        }
    }

    if (!rarest)
        return nullptr;

    missingPieces.erase(
            std::remove(missingPieces.begin(), missingPieces.end(), rarest),
            missingPieces.end()
    );
    ongoingPieces.push_back(rarest);
    return rarest;
}

bool PieceManager::write(Piece* piece)
{
    const std::string data = piece->getData();
    std::lock_guard<std::mutex> guard(fileLock);
    downloadedFile.seekp((std::streamoff)piece->index * pieceLength);
    downloadedFile.write(data.data(), (std::streamsize)data.size());
    downloadedFile.flush();
    return downloadedFile.good();
}

void PieceManager::blockReceived(std::string peerId, int pieceindex, int blockOffset, std::string data)
{
    SPDLOG_INFO("Recieved block {} for piece {} from peer {}",blockOffset, pieceindex, peerId);

    std::unique_lock<std::mutex> guard(lock);

    for( auto it = pendingReqs.begin(); it != pendingReqs.end();)
    {
        if ((*it)->block->piece == pieceindex && (*it)->block->offset == blockOffset)
        {
            delete *it;
            it = pendingReqs.erase(it);
        }else
        {
            it++;
        }
    }

    Piece* targetPiece = nullptr;
    for (Piece* p : ongoingPieces)
    {
        if(p->index == pieceindex)
        {
            targetPiece = p;
            break;
        }
    }
    if(!targetPiece) return;

    if(!targetPiece->blockReceived(blockOffset, std::move(data))) return;
    if (!targetPiece->isComplete()) return;

    ongoingPieces.erase(std::remove(ongoingPieces.begin(), ongoingPieces.end(),targetPiece), ongoingPieces.end());
    guard.unlock();
    const bool hashOk = targetPiece->isMatchingHash();
    const bool written = hashOk && write(targetPiece);
    guard.lock();

    if(hashOk && written)
    {
        targetPiece->releaseData();
        havePieces.push_back(targetPiece);
        piecesDownloadedInInterval++;
        SPDLOG_INFO("({:.2f}%) {} / {} Pieces Downloaded...", 100.0f * havePieces.size() /totalPieces,  havePieces.size(), totalPieces);

    }else{
        targetPiece->reset();
        ongoingPieces.push_back(targetPiece);
        if (hashOk) writeFailed = true;
        else SPDLOG_INFO("Hash Mismatch for piece {}", targetPiece->index);
    }


}

unsigned long PieceManager::bytesDownloaded()
{
    lock.lock();
    unsigned long bytesDownloaded = havePieces.size() * pieceLength;
    lock.unlock();
    return bytesDownloaded;
}

void PieceManager::trackProgress()
{
    usleep(pow(10, 6));
    while (!isComplete())
    {
        displayProgressBar();
        piecesDownloadedInInterval = 0;
        usleep(PROGRESS_DISPLAY_INTERVAL * pow(10,6));
    }
}

void PieceManager::displayProgressBar()
{
    std::stringstream info;
    lock.lock();
    unsigned long downloadedPieces = havePieces.size();
    unsigned long downloadedLength = pieceLength * piecesDownloadedInInterval;

    double avgDownloadSpeed = (double) downloadedLength / (double) PROGRESS_DISPLAY_INTERVAL;
    double avgDownloadSpeedInMBS = avgDownloadSpeed / pow(10, 6);

    info << "[Peers: " + std::to_string(peers.size()) + "/" + std::to_string(maxConnections) + ", ";
    info << std::fixed << std::setprecision(2) << avgDownloadSpeedInMBS << " MB/s, ";

    double timePerPiece = (double) PROGRESS_DISPLAY_INTERVAL / (double) piecesDownloadedInInterval;
    long remainingTime = ceil(timePerPiece * (totalPieces - downloadedPieces));
    info << "ETA: " << formatTime(remainingTime) << "]";

    double progress = (double) downloadedPieces / (double) totalPieces;
    int pos = PROGRESS_BAR_WIDTH * progress;
    info << "[";
    for (int i = 0; i < PROGRESS_BAR_WIDTH; i++)
    {
        if (i < pos) info << "=";
        else if (i == pos) info << ">";
        else info << " ";
    }
    info << "] ";
    info << std::to_string(downloadedPieces) + "/" + std::to_string(totalPieces) + " ";
    info << "[" << std::fixed << std::setprecision(2) << (progress * 100) << "%] ";

    time_t currentTime = std::time(nullptr);
    long timeSinceStart = floor(std::difftime(currentTime, startingTime));

    info << "in " << formatTime(timeSinceStart);
    std::cout << info.str() << "\r";
    std::cout.flush();
    lock.unlock();
    if (isComplete())
        std::cout << std::endl;
}
