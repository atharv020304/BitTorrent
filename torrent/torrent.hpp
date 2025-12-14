#pragma once
#include "parser\Bencode.cpp"

struct TorrentMeta
{
    string announce;
    string info_hash;
    long long length = 0;
    string name;
};

TorrentMeta parseTorrent(const string &path);