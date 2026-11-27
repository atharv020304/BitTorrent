/**
 * @file connect.h
 * @brief Low-level TCP socket helpers used by peer connections.
 */

#ifndef TORRENTCONNECT_H
#define TORRENTCONNECT_H
#include <string>
#include <cstdint>

/** @brief Opens a blocking TCP connection to a peer. */
int createConnection(const std::string& ip, int port);

/** @brief Sends raw bytes on an open socket. */
void sendData(int sock, const std::string& data);

/** @brief Reads a length-prefixed BitTorrent message from a socket. */
std::string receiveData(int sock, uint32_t bufferSize = 0);

class ReadTimeout : public std::runtime_error  { public : using std::runtime_error::runtime_error; };

#endif /*TORRENTCONNECT_H*/
