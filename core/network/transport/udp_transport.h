//UDP传输类声明

#ifndef UDP_TRANSPORT_H
#define UDP_TRANSPORT_H

#include <sys/socket.h>
#include <cstdint>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <string.h>

class UdpTransport {
public:
    UdpTransport() : sockfd_(-1), running_(false) {}
    ~UdpTransport();
    
    bool Initialize(uint16_t port);
    ssize_t SendTo(const struct sockaddr_in& dest, const void* data, size_t len);
    ssize_t RecvFrom(void* buf, size_t len, struct sockaddr_in* src_addr);
    ssize_t SendWithAck(const sockaddr_in& dest, const void* data, size_t len, uint32_t seq_num, int max_retries = 3);
    void StartAckListener();
    void StopAckListener() { running_ = false; }

private:
    int sockfd_;
    std::atomic_bool running_;
    bool WaitForAck(uint32_t seq_num);
};

#endif