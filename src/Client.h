/**
 * @file Client.h
 * @brief Top-level client that coordinates tracker queries, peer workers, and piece assembly.
 */

#ifndef TORRENT_CLIENT_H
#define TORRENT_CLIENT_H


#include <string>
#include <vector>
#include <thread>
#include <memory>
#include <spdlog/spdlog.h>
#include "RetrivePeers.h"
#include "ConnectToPeer.h"
#include "SharedQueue.h"

/**
 * @class TorrentClient
 * @brief Orchestrates a multi-threaded BitTorrent download.
 *
 * Creates worker threads (@ref PeerConnection), periodically fetches peers from
 * the tracker (@ref PeerRetriever), and delegates piece scheduling to @ref PieceManager.
 */
class TorrentClient
{
    private:
        const int threadNum;
        std::string peerId;
        SharedQueue<Peer*> queue;
        std::vector<std::thread> threadPool;
        std::vector<PeerConnection*> connections;
        std::shared_ptr<spdlog::logger> logger;
    public:
    explicit TorrentClient(int threadNum = 5, bool enableLogging = true, std::string logFilePath = "logs/client.log");
    ~TorrentClient();
    void terminate();
    void downloadFile(const std::string& torrentFilePath, const std::string& downloadDirectory);

};
#endif 