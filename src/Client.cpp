#include <random>
#include <iostream>
#include <thread>
#include <bencode/bencoding.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "PeerConnection.h"
#include "Client.h"
#include "TorrentFileParser.h"
#include "RetrivePeers.h"
#include "ConnectToPeer.h"

#define PORT 8081
#define INTERVAL_Q = 60

TorrentClient::TorrentClient(const int threadNum, bool log, std::string LogFilePath) : threadNum(threadNum)
{
    peerId = "-UT2021-";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> disturb(1, 9);
    for (int i = 0; i < 12; i++)
    {
        peerId += std::to_string(disturb(gen));
    }

    if (log)
    {
        logger = spdlog::basic_logger_mt("file_logger", logFilePath, true); // true = truncate
        logger->set_level(spdlog::level::trace);                            // Log everything
        logger->flush_on(spdlog::level::trace);                             // Flush after every message
        spdlog::set_default_logger(logger);
    }
    else
    {
        // Disable all logging
        spdlog::set_level(spdlog::level::off);
    }
}

TorrentClient::~TorrentClient() = default;

void TorrentClient::downloadFile(const std::string& torrentFilePath, const std::string& downloadDirectory)
{

    std::cout << "Parsing Torrent file " + torrentFilePath + "..." << std::endl;
    TorrentFileParser torrentFileParser(torrentFilePath);
    std::string announceUrl = torrentFileParser.getAnnounce();

    long fileSize = torrentFileParser.getFileSize();
    const std::string infoHash = torrentFileParser.getInfoHash();

    std::string filename = torrentFileParser.getFileName();
    std::string downloadPath = downloadDirectory + filename;
    PieceManager pieceManager(torrentFileParser, downloadPath, threadNum);

    for (int i = 0; i < threadNum; i++)
    {
        PeerConnection connection(&queue, peerId, infoHash, &pieceManager);
        connections.push_back(&connection);
        std::thread thread(&PeerConnection::start, connection);
        threadPool.push_back(std::move(thread));
    }

    auto lastPeerQuery = (time_t) (-1);

    std::cout << "Download initiated..." << std::endl;

    while (true)
    {
        if (pieceManager.isComplete())
            break;

        time_t currentTime = std::time(nullptr);
        auto diff = std::difftime(currentTime, lastPeerQuery);
       
        if (lastPeerQuery == -1 || diff >= PEER_QUERY_INTERVAL || queue.empty())
        {
            PeerRetriever peerRetriever(peerId, announceUrl, infoHash, PORT, fileSize);
            std::vector<Peer*> peers = peerRetriever.retrievePeers(pieceManager.bytesDownloaded());
            lastPeerQuery = currentTime;
            if (!peers.empty())
            {
                queue.clear();
                for (auto peer : peers)
                    queue.push_back(peer);

            }
        }
    }

    terminate();

    if (pieceManager.isComplete())
    {
        std::cout << "Download completed!" << std::endl;
        std::cout << "File downloaded to " << downloadPath << std::endl;
    }
}

void TorrentClient::terminate()
{
    
    for (int i = 0; i < threadNum; i++)
    {
        Peer* dummyPeer = new Peer { "0.0.0.0", 0 };
        queue.push_back(dummyPeer);
    }
    for (auto connection : connections)
        connection->stop();

    for (std::thread& thread : threadPool)
    {
        if (thread.joinable())
            thread.join();
    }

    threadPool.clear();
}
