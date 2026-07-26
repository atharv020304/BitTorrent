/**
 * @file utils.h
 * @brief Shared encoding, bitfield, and formatting helpers.
 */

#ifndef TORRENT_UTILS_H
#define TORRENT_UTILS_H

#include <string>

/** @brief Percent-encodes a string for HTTP tracker requests. */
std::string urlEncode(const std::string& value);

/** @brief Converts a hex string into raw bytes. */
std::string hexDecode(const std::string& value);

/** @brief Converts raw bytes into a hex representation. */
std::string hexEncode(const std::string& input);

/** @brief Returns true when the peer bitfield marks a piece as available. */
bool hasPiece(const std::string& bitField, int index);

/** @brief Marks a piece as available in a bitfield. */
void setPiece(std::string& bitField, int index);

/** @brief Converts a big-endian byte string to an integer. */
int bytesToInt(std::string bytes);

/** @brief Formats seconds as `mm:ss` or `hh:mm:ss`. */
std::string formatTime(long seconds);

#endif
