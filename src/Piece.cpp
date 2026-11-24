
#include <bits/stdc++.h>
#include <crypto/sha1.h>
#include <spdlog/spdlog.h>

#include "utils.h"
#include "Piece.h"

Piece::Piece(int index, std::vector<Block *> blk, std::string hVal) : index(index),
                                                                      hVal(hVal)
{
    this->blk = std::move(blk);
}

Piece::~Piece()
{
    for (auto b : blk)
    {
        delete b;
    }
}

Block *Piece::nextRequest()
{
    for (Block *b : blk)
    {
        if (b->status == missing)
        {
            b->status = pending;
            return b;
        }
    }
    return NULL;
}

bool Piece::blockReceived(int offset, std::string data)
{
    for (Block *b : blk)
    {
        if (b->offset == offset)
        {   
            if(b->status == retrieved || (int)data.size() != b->length) return false;
            b->status = retrieved;
            b->data = std::move(data);
            return true;
        }
    }
    return false;
}

bool Piece::isComplete()
{
    for (Block *block : blk)
    {
        if (block->status != retrieved)
        {
            return false;
        }
    }

    return true;
}

bool Piece::isMatchingHash()
{
    std::string pHash = hexDecode(sha1(getData()));
    return pHash == hVal;
}

std::string Piece::getData()
{
    assert(isComplete());
    std::stringstream data;
    for(Block* b : blk)
    {
        data << b->data;
    }
    return data.str();
}

void Piece::reset()
{
    for(Block* b : blk)
    {
        b->status = missing;
    }
}