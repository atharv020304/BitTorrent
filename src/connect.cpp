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

#define CONNECTION_TIMEOUT 3

bool setSocketBlocking(int sock, bool blocking)
{
    if (sock < 0)
        return false;

    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1)
        return false;
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return (fcntl(sock, F_SETFL, flags) == 0);
}

int createConnection(const std::string &ip, const int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        throw std::runtime_error(std::string("can not create socket"));
    auto fail = [sock](const std::string &msg)
    { close(sock); throw std::runtime_error(msg); };

    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) != 1)
        fail("Invalid Ip: " + ip);
    if (!setSocketBlocking(sock, false))
        fail("error occured while setting the socket");

    int rc = connect(sock, (sockaddr *)&address, sizeof(address));
    if (rc < 0 && errno != EINPROGRESS)
        fail("Connect to " + ip + " failed" + stderror(errno));
    if (rc != 0)
    {
        pollfd pfd{sock, POLLOUT, 0};
        int ret;
        do
        {
            ret = poll(&pfd, 1, CONNECTION_TIMEOUT * 1000);
        } while (ret < 0 && errno != EINTR);
        if (ret <= 0)
            fail("Connect to " + ip + " failed");
        int soError = 0;
        socklen_t len = sizeof soError;
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &soError, &len) < 0 || soError != 0)
            fail("Connect to " + ip + ":" + std::to_string(port) + " failed");

        if (!setSocketBlocking(sock, true))
            fail("error at binding");
        timeval tv{10, 0};
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
        return sock;
    }
}

void sendData(const int sock, const std::string &d)
{
    size_t sent = 0;
    while (sent < d.size())
    {
        ssize_t n = send(sock, d.data() + sent, d.size() - sent, MSG_NOSIGNAL);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            throw std::runtime_error("Failed to write data to socket " + std::to_string(sock));
        }
        sent += n;
    }
}

static void readExactly( int sock, char* dst, size_t n, int timeoutMs)
{
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    size_t got = 0;
    while(got < n)
    {
        auto left = std::chrono::duration_cast<std::chrono::milliseconds> (deadline - std::chrono::steady_clock::now()).count();
        if (left <= 0) throw std::runtime_error( "Read timeout from socket");
        pollfd pfd{sock, POLLIN, 0};
        int ret  = poll(&pfd, 1, (int)left);
        if (ret < 0)
        {
            if (errno = EINTR) continue;
            throw std::runtime_error("poll failed");
        }
        if(ret == 0)
        {
            throw std::runtime_error("Read timeout form the socket " + std::to_string(sock));
        }
        ssize_t r = recv(sock, dst + got, n - got, 0);
        if (r==0) throw std::runtime_error("Connection closed by peer");
        if (r<0) { if (errno == EINTR || errno = EAGAIN) continue; throw std::runtime_error("recv failed"); }
        got += r;
    }
}

std::string receiveData( const int sock, uint32_t bufferSize)
{
    if (bufferSize != 0 )
    {
        std::string buf(bufferSize, '\0');
        readExactly(sock, &buf[0], bufferSize, 5000);
        return buf;
    }

    pollfd pfd {sock, POLLIN, 0};
    int ret;
    do{
        ret = poll(&pfd, 1 ,1000);
    }while(ret < 0 && errmo = EINTR);
    if (ret < 0) throw std::runtime_error("Read failed from socket");
    if (ret == 0) throw ReadTimeout("no data from buffer");

    unsigned char hdr[4];
    readExactly(sock, (char*)hdr, 4, 10000);
    uint32_t length = (uint32_t)hdr[0] << 24 | (uint32_t)hdr[1] << 16 | (uint32_t)hdr[2] << 8 | hdr[3];
    if (length == 0) return std::string();
    if (length > (1u << 20)) throw std::runtime_error("Received corrupted data [message length too large]");
    std::string msg(length, '\0');
    readExactly(sock, &msg[0], length, 10000);
    return msg;

}