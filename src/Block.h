#ifndef BITTORRENTCLIENT_BLOCK_H
#define BITTORRENTCLIENT_BLOCK_H

enum Status
{
    missing = 0,
    pending = 1,
    retrieved = 2
};

struct Block
{
    int piece;
    int offset;
    int length;
    Status status;
    std::string data;
};

#endif