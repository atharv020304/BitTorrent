/**
 * @file TorrentFileParser.h
 * @brief Parses `.torrent` metadata using the bencode library.
 */

#ifndef TORRENTFILEPARSER_H
#define TORRENTFILEPARSER_H

#include <memory>
#include <string>
#include <vector>
#include <bencode/BDictionary.h>

using byte = unsigned char;

/**
 * @class TorrentFileParser
 * @brief Loads torrent metadata and exposes announce URL, info hash, and piece hashes.
 */
class TorrentFileParser
{
    private: 
        std::shared_ptr<bencoding::BDictionary> root;

    public:
        explicit TorrentFileParser( const std::string& path);
        long getFileSize() const;
        long getPieceLength() const;
        std::string getFileName() const;
        std::string getAnnounce() const;
        std::shared_ptr<bencoding::BItem> get(std::string key) const;
        std::string getInfoHash() const;
        std::vector<std::string> splitPieceHashes() const;
};

#endif /*TORRENTFILEPARSER_H*/
