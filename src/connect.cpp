#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <limits>
#include <spdlog/spdlog.h>
#include "connect.h"

#define TIMEOUT_FOR_READ 3000
#define CONNECTION_TIMEOUT 3


bool setSocketBlocking(int sock, bool blocking)
{
    if (sock < 0)
        return false;

    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) return false;
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return (fcntl(sock, F_SETFL, flags) == 0);
}

int createConnection(std::string& ip, const int port)
{
    int sock = 0;
    struct sockaddr_in address;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        throw std::runtime_error("cannot create socket" + std::to_string(sock));

    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    char* tempIp = new char[ip.length()+1];
    strcpy(tempIp,ip.c_str());

    // convert ip from string to struct in_addr
    if (inet_pton(AF_INET, tempIp, &address.sin_addr) <= 0)
        throw std::runtime_error("Invalid Ip");

    if(!setSocketBlocking(sock, false))
        throw std::runtime_error("error occured while setting the socket");

    connect(sock, (struct sockaddr*)& address, sizeof(address));

    fd_set fs;
    struct timeval tv;

    FD_ZERO(&fs);
    FD_SET(sock,&fs);
    tv.tv_sec = CONNECTION_TIMEOUT;
    tv.tv_usec = 0;

    if(select(sock+1, NULL, &fs, NULL, &tv) == 1)
    {
    int so_error;
    socklen_t len = sizeof so_error;
    
    getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);

    if(so_error == 0)
    {
        //set to blocking mode
        if(!setSocketBlocking(sock,true))
            throw std::runtime_error("error at binding");
        return sock;
    }
    }
    close(sock);
    throw std::runtime_error("Connect to" + ip + ":Failed  [Connection timeout] ");
}