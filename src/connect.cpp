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
#include "utils.h"

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

int createConnection(const std::string& ip, const int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) throw std::runtime_error(std::string("can not create socket"));
    auto fail = [sock](const std::string& msg) { close(sock); throw std::runtime_error(msg); };

    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if(inet_pton(AF_INET, ip.c_str(), &address.sin_addr) != 1) fail("Invalid Ip: " + ip);
    if(!setSocketBlocking(sock, false)) fail("error occured while setting the socket");

    int rc = connect(sock, (sockaddr*)&address, sizeof(address));
    if( rc < 0 && errno != EINPROGRESS ) fail("Connect to "+ ip +" failed" + stderror(errno));
    if(rc != 0)
    {
        pollfd pfd{sock, POLLOUT, 0};
        int ret; do { ret = poll(&pfd, 1 , CONNECTION_TIMEOUT * 1000); } while (ret < 0 && errno != EINTR);
        if(ret <= 0) fail("Connect to "+ ip + " failed");
        int soError = 0;
        socklen_t len = sizeof soError;
        if(getsockopt(sock, SOL_SOCKET, SO_ERROR, &soError, &len) < 0 || soError != 0)
            fail("Connect to " + ip + ":" + std::to_string(port) + " failed");
        
        if(!setSocketBlocking(sock, true)) fail("error at binding");
        timeval tv{10,0};
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
        return sock;
    }
}

void sendData(const int sock, const std::string& d)
{
    int n = d.length();
    char buff[n];
    for(int i=0;i<n;i++){
        buff[i] = d[i];
    }

    int res = send(sock,buff,n,0);
    if (res < 0)
        throw std::runtime_error("Failed to write data to socket " + std::to_string(sock));
}

std::string receiveData(const int sock, uint32_t bufferSize)
{
    std::string r;

    // If buffer size is not provided, read the first 4 bytes
    // which contain the incoming message length.
    if (!bufferSize)
    {
        struct pollfd fd;
        int ret;
        fd.fd = sock;
        fd.events = POLLIN;

        ret = poll(&fd, 1, 3000);

        long bytesRead = 0;
        const int lenIndSize = 4;
        char buff[lenIndSize];

        switch (ret)
        {
            case -1:
                throw std::runtime_error("Read failed from socket");

            case 0:
                throw std::runtime_error("Read failed from timeout");

            default:
                bytesRead = recv(sock, buff, sizeof(buff), 0);
                break;
        }

        // Length indicator must be exactly 4 bytes.
        if (bytesRead != lenIndSize)
            return r;

        // Convert received length bytes into message size.
        std::string messageLenStr;
        for (char i : buff)
            messageLenStr += i;

        uint32_t messageLength = bytesToInt(messageLenStr);
        bufferSize = messageLength;
    }

    // Reject invalid message sizes larger than supported limit.
    if (bufferSize > std::numeric_limits<uint16_t>::max())
        throw std::runtime_error(
            "Received corrupted data [Received buffer size greater than 2 ^ 16 - 1]");

    char buffer[bufferSize];

    long bytesRead = 0;
    long bytesToRead = bufferSize;

    auto startTime = std::chrono::steady_clock::now();

    do
    {
        // Abort if overall receive operation exceeds timeout.
        auto diff = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration<double, std::milli>(diff).count() > TIMEOUT_FOR_READ)
        {
            throw std::runtime_error(
                "Read timeout from socket " + std::to_string(sock));
        }

        // Read only the remaining bytes still expected.
        bytesRead = recv(
            sock,
            buffer + (bufferSize - bytesToRead),
            bytesToRead,
            0);

        if (bytesRead <= 0)
        {
            throw std::runtime_error(
                "Failed to receive data from socket " + std::to_string(sock));
        }

        bytesToRead -= bytesRead;
    }
    while (bytesToRead > 0);

    // Build response string from the fully received buffer.
    r.assign(buffer, bufferSize);

    return r;
}