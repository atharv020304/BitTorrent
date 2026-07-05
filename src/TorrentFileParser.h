#ifndef TORRENTFILEPARSER_H
#define TORRENTFILEPARSER_H

#include<bits/stdc++.h>
#include <bencode/BDictionary.h>

using byte = unsigned char;

//class used to parse the given torrent file using bencode library.

class TorrentFileParser
{
    private: 
        std::shared_ptr<bencoding::BDictionary> root;

    public:
        explicit TorrentFileParser( std::string& path);
        long getFileSize() const;
        long getPieceLength() const;
        std::string getFileName() const;
        std::string getAnnounce() const;
        std::shared_ptr<bencoding::BItem> get(std::string key) const;
        std::string getInfoHash() const;
        std::vector<std::string> splitPieceHashes() const;
};

#endif /*TORRENTFILEPARSER_H*/