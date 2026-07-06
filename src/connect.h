#ifndef TORRENTCONNECT_H
#define TORRENTCONNECT_H
#include <string>

int createConnection(const std::string& ip, int port);
void sendData(int sock, const std::string& data);
std::string recieveData(int sock, uint32_t bufferSize = 0);

#endif /*TORRENTCONNECT_H*/