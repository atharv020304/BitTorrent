#include <bits/stdc++.h>
#include <lib/spdlog/spdlog.h>
#include <unistd.h>
#include "PieceManager.h"
#include "Block.h"
#include "utils.h"

#define B_SIZE 16382
#define MAX_PENDING_TIME 5
#define PROGRESS_BAR_WIDTH 40
#define PROGRESS_DISPLAY_INTERVAL 1

PieceManager::PieceManager(
    const TorrentFileParser& fileParser,
    const std::string& dPath,
    const int maxConnections
): fileParser(fileParser), maxConnections(maxConnections), pieceLength(fileParser.getPieceLength())
{
    missingPieces = initPieces();
    // create a destination file with specified size
    downloadedFile.open(dPath, std::ios::binary | std::ios::out);
    downloadedFile.seekp(fileParser.getFileSize() - 1);
    downloadedFile.write("",1);

    startingTime = std::time(nullptr);
    std::thread progressThread([this] { this->trackProgress(); } );
    progressThread.detach();
}

PieceManager::~PieceManager() {
    for(Piece* p: missingPieces)
        delete p;
    
    for(Piece* p: ongoingPieces)
        delete p;
    
    for(pRequest* p : pendingReqs)
        delete p;

    downloadedFile.close();

}

std::vector<Piece*> PieceManager::initPieces()
{
    std::vector<std::string> pieceHashes = fileParser.splitPieceHashes();
    totalPieces = pieceHashes.size();
    std::vector<Piece*> torrentPieces;
    missingPieces.reserve(totalPieces);

    long totalLength = fileParser.getFileSize();

    int blockCount = ceil(pieceLength / B_SIZE);
    long remLength = pieceLength;

    for(int i=0; i< totalPieces; i++)
    {
        if(i == totalPieces - 1)
        {
            remLength = totalLength % pieceLength;
            blockCount = std::max( (int) ceil(remLength /B_SIZE));
        }

            std::vector<Block* >blocks;
            blocks.reserve(blockCount);

            for(int offset = 0 ; offset < blockCount; offset++){
                Block* block = new Block;
                block->piece = i;
                block->status = missing;
                block->offset = offset * B_SIZE;
                int blockSize = B_SIZE;
                if( i == totalPieces - 1 && offset == blockCount - 1 )
                {
                    blockSize = remLength % B_SIZE;
                }
                block->length = blockSize;
                blocks.push_back(block);
            }
        auto piece = new Piece(i,blocks,pieceHashes[i]);
        torrentPieces.emplace_back(piece);
    }
    return torrentPieces;
}

