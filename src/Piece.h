/**
 * @file Piece.h
 * @brief In-memory representation of one torrent piece composed of blocks.
 */

#ifndef TORRENT_PIECE_H
#define TORRENT_PIECE_H

#include "Block.h"
#include <vector>

/**
 * @class Piece
 * @brief Aggregates downloaded blocks and verifies the piece SHA-1 hash.
 */
class Piece
{
    private:
        const std::string hVal;

    public:
        const int index;
        std::vector<Block*> blk;

        explicit Piece(int index, std::vector<Block*> blk, std::string hVal);
        std::string getData();
        Block* nextReq();
        void blockReceived(int offset, std::string data);
        bool isComplete();
        bool isMatchingHash();
        ~Piece();
        void reset();

};

#endif
