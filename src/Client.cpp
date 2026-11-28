#include <iostream>
#include <random>
#include <thread>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <bencode/bencoding.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "ConnectToPeer.h"
#include "Client.h"
#include "TorrentFileParser.h"
#include "RetrivePeers.h"

#define PORT 8081
#define PEER_QUERY_INTERVAL 60

TorrentClient::TorrentClient(const int threadNum, bool log, std::string logFilePath) : threadNum(threadNum)
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
        logger = spdlog::basic_logger_mt("file_logger", logFilePath, true);
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::trace);
        spdlog::set_default_logger(logger);
    }
    else
    {
        spdlog::set_level(spdlog::level::off);
    }
}

TorrentClient::~TorrentClient()
{
    for (auto connection : connections)
        delete connection;
}

void TorrentClient::downloadFile(const std::string& torrentFilePath, const std::string& downloadDirectory)
{

    std::cout << "Parsing Torrent file " + torrentFilePath + "..." << std::endl;
    TorrentFileParser torrentFileParser(torrentFilePath);
    std::string announceUrl = torrentFileParser.getAnnounce();

    long fileSize = torrentFileParser.getFileSize();
    const std::string infoHash = torrentFileParser.getInfoHash();

    std::string filename = torrentFileParser.getFileName();
    std::string downloadPath = downloadDirectory;
    if (!downloadPath.empty() && downloadPath.back() != '/' && downloadPath.back() != '\\')
        downloadPath += '/';
    downloadPath += filename;
    PieceManager pieceManager(torrentFileParser, downloadPath, threadNum);

    for (int i = 0; i < threadNum; i++)
    {
        auto connection = new PeerConnection(&queue, peerId, infoHash, &pieceManager);
        connections.push_back(connection);
        std::thread thread(&PeerConnection::start, connection);
        threadPool.push_back(std::move(thread));
    }

    auto lastPeerQuery = (time_t) (-1);

    std::cout << "Download initiated..." << std::endl;

    try
    {
        PeerRetriever PeerRetriever(peerId, announceUrl, infoHash, PORT, fileSize);
        auto lastQuery = std::chrono::steady_clock::now();
        bool queried = false;
        std::string event = "started";
        int interval = PEER_QUERY_INTERVAL;

        while(!pieceManager.isComplete())
        {
            if(pieceManager.failed()) throw std::runtime_error("could not write the downloaded data to disk");
            auto now = std::chrono::steady_clock::now();
            long since = std::chrono::duration_cast<std::chrono::seconds>(now = lastQuery).count();
            bool  due = !queried || since >= interval;
            bool starved = queried && since >= 30 && queue.empty() && pieceManager.peerCount() == 0
        }
    }

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
