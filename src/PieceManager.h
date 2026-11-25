
#ifndef TORRENT_MANAGE_PIECES_H
#define TORRENT_MANAGE_PIECES_H

#include <map>
#include <vector>
#include <ctime>
#include <mutex>
#include <fstream>
#include <thread>
#include <atomic>
#include "Piece.h"
#include "TorrentFileParser.h"
#include <condition_variable>

struct pRequest
{
    Block *block;
    time_t timestamp;
};

class PieceManager
{
private:
    std::map<std::string, std::string> peers;
    std::vector<Piece *> missingPieces;
    std::vector<Piece *> ongoingPieces;
    std::vector<Piece *> havePieces;
    std::vector<pRequest*> pendingReqs;
    std::ofstream downloadedFile;

    const long pieceLength;
    const TorrentFileParser &fileParser;
    const int maxConnections;
    int piecesDownloadedInInterval = 0;
    time_t startingTime;
    int totalPieces{};

    // lock to prevent race conds
    std::mutex lock;
    std::mutex fileLock;
    std::atomic<bool> writeFailed{false};
    std::thread progressThread;
    std::mutex progressMutex;
    std::condition_variable progressCv;
    bool stopProgress = false;

    std::vector<Piece*> initPieces();
    Block *expiredRequest(std::string peerId);
    Block *nextOngoing(std::string peerId);
    Piece *getRarestPiece(std::string peerId);
    bool write(Piece *piece);
    void displayProgressBar();
    void trackProgress();

public:
    explicit PieceManager(const TorrentFileParser &fileParser, const std::string &downloadPath, int maxConnections);
    ~PieceManager();
    bool isComplete();
    void blockReceived(std::string peerId, int pieceIndex, int blockOffset, std::string data);
    void addPeer(const std::string &peerId, std::string bitField);
    void removePeer(const std::string &peerId);
    void updatePeer(const std::string &peerId, int index);
    bool failed() const { return writeFailed.load() };
    unsigned long bytesDownloaded();
    Block *nextRequest(std::string peerId);
};

#endif /**/