#include <regex>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include <bencode/BItem.h>
#include <bencode/Decoder.h>
#include <bencode/bencoding.h>
#include <crypto/sha1.h>
#include <spdlog/spdlog.h>
#include "TorrentFileParser.h"

#define HASH_LEN 20


TorrentFileParser::TorrentFileParser(const std::string& path)
{
    SPDLOG_INFO("Parsing Torrent File %s...",path.c_str());
    std::ifstream fileStream(path, std::ifstream::binary);
    if (!fileStream.is_open()) throw std::runtime_error("Cannot open torrent file '" + path + "'");
    root = std::dynamic_pointer_cast<bencoding::BDictionary> (bencoding::decode(fileStream));
    if(!root) throw std::runtime_error("Not a valid torrent file");
    SPDLOG_INFO("Parsing Successful");
}

std::shared_ptr<bencoding::BItem> TorrentFileParser::get(std::string key) const {
    std::shared_ptr<bencoding::BItem> infoDict = root->getValue("info");
    if (!infoDict)
        throw std::runtime_error("Torrent file is malformed. [File does not contain key 'info']");
    auto info = std::dynamic_pointer_cast<bencoding::BDictionary>(infoDict);
    if(!info) throw std::runtime_error("torrent file is malformed.");
    std::shared_ptr<bencoding::BItem> value = info->getValue(key);
    return value;
}

std::string TorrentFileParser::getInfoHash() const{
    std::shared_ptr<bencoding::BItem> infoDict = root->getValue("info");
    if(!infoDict) throw std::runtime_error("torrent file is malformed");
    std::string infoString = bencoding::encode(infoDict);
    std::string sha1Hash = sha1(infoString);
    return sha1Hash;
}

std::vector<std::string> TorrentFileParser::splitPieceHashes() const
{
    std::shared_ptr<bencoding::BItem> piecesValue = get("pieces");
    auto ps = std::dynamic_pointer_cast<bencoding::BString>(piecesValue);
    if (!ps) throw std::runtime_error("Malformed Torrent File");
    std::string pieces = ps->value();
    if(pieces.size() % HASH_LEN != 0 ) throw std::runtime_error("Malformed Torrent file");

    std::vector<std::string> pieceHashes;

    assert(pieces.size() % HASH_LEN == 0);

    int piecesCount = (int) pieces.size() / HASH_LEN;
    pieceHashes.reserve(piecesCount);
    for( int i = 0 ; i < piecesCount; i++)
        pieceHashes.push_back(pieces.substr(i * HASH_LEN,HASH_LEN));
    
    return pieceHashes;
}

long TorrentFileParser::getFileSize() const{
    if(get("files")) throw std::runtime_error("Multi-File torrents are not supported");
    std::shared_ptr<bencoding::BItem> fileSizeItem = get("length");
    if(!fileSizeItem)
        throw std::runtime_error("Torrent File is Malformed");
    long fileSize = std::dynamic_pointer_cast<bencoding::BInteger>(fileSizeItem)->value();
    return fileSize;
}

long TorrentFileParser::getPieceLength() const{
    std::shared_ptr<bencoding::BItem> pieceLengthItem = get("piece length");
    if(!pieceLengthItem)
        throw std::runtime_error("Torrent file is malformed ");
    long pieceLength = std::dynamic_pointer_cast<bencoding::BInteger>(pieceLengthItem)->value();
    return pieceLength;
}

std::string TorrentFileParser::getFileName() const{
    std::shared_ptr<bencoding::BItem> filenameItem = get("name");
    if (!filenameItem)
        throw std::runtime_error("Torrent file is malformed. [File does not contain key 'name']");
    std::string filename = std::dynamic_pointer_cast<bencoding::BString>(filenameItem)->value();
    return filename;
}

std::string TorrentFileParser::getAnnounce() const
{
    std::shared_ptr<bencoding::BItem> announceItem = root->getValue("announce");
    if (!announceItem)
        throw std::runtime_error("Torrent file is malformed. [File does not contain key 'announce']");
    std::string announce = std::dynamic_pointer_cast<bencoding::BString>(announceItem)->value();
    return announce;
}
