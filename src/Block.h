/**
 * @file Block.h
 * @brief Fixed-size chunk within a torrent piece.
 */

#ifndef BITTORRENTCLIENT_BLOCK_H
#define BITTORRENTCLIENT_BLOCK_H

#include <string>

/**
 * @enum Status
 * @brief Download state of an individual block.
 */
enum Status
{
    missing = 0,
    pending = 1,
    retrieved = 2
};

/**
 * @struct Block
 * @brief Describes one block request within a piece.
 */
struct Block
{
    int piece;
    int offset;
    int length;
    Status status;
    std::string data;
};

#endif
