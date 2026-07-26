#ifndef TORRENTCONNECT_H
#define TORRENTCONNECT_H
#include <string>
#include <cstdint>

int createConnection(const std::string& ip, int port);
void sendData(int sock, const std::string& data);
std::string receiveData(int sock, uint32_t bufferSize = 0);

#endif /*TORRENTCONNECT_H*/
