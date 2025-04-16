//udp传输实现

#include "udp_transport.h"
#include <stdexcept>
#include <fcntl.h>
#include <stdio.h>
#include <netinet/in.h>  // 定义sockaddr_in和INADDR_ANY
#include <sys/socket.h>  // 定义socket相关函数
#include <chrono>
#include <sys/select.h>
#include <errno.h>

#define LOG_ERROR(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  fprintf(stderr, "[WARN] " fmt "\n", ##__VA_ARGS__)

UdpTransport::~UdpTransport() {
    StopAckListener();
    if (sockfd_ != -1) close(sockfd_);
}

bool UdpTransport::Initialize(uint16_t port) {
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ == -1) return false;
    
    // 设置为非阻塞模式
    int flags = fcntl(sockfd_, F_GETFL, 0);
    if (fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        close(sockfd_);
        return false;
    }
    
    //、结构体初始化
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    return bind(sockfd_, (struct sockaddr*)&addr, sizeof(addr)) == 0;
}

ssize_t UdpTransport::SendTo(const struct sockaddr_in& dest, const void* data, size_t len) {
    return sendto(sockfd_, data, len, 0, (struct sockaddr*)&dest, sizeof(dest));
}

ssize_t UdpTransport::RecvFrom(void* buf, size_t len, struct sockaddr_in* src_addr) {
    socklen_t addr_len = sizeof(*src_addr);
    return recvfrom(sockfd_, buf, len, 0, (struct sockaddr*)src_addr, &addr_len);
}

void UdpTransport::StartAckListener() {
    running_ = true;
    std::thread([this]() {
        fd_set read_fds;
        while (running_) {
            FD_ZERO(&read_fds);
            FD_SET(sockfd_, &read_fds);
            struct timeval tv = {1, 0};
            if (select(sockfd_ + 1, &read_fds, nullptr, nullptr, &tv) > 0) {
                //处理接收数据或ACK
            }
        }
    }).detach();
}

ssize_t UdpTransport::SendWithAck(const sockaddr_in& dest, const void* data, size_t len, uint32_t seq_num, int max_retries) {
    for (int i = 0; i < max_retries; ++i) {
        ssize_t sent = SendTo(dest, data, len);
        if (sent < 0) {
            LOG_ERROR("Send failed: %s", strerror(errno));
            continue;
        }
        if (WaitForAck(seq_num)) {
            return sent;
        }
        LOG_WARN("ACK timeout, retrying...");
        usleep(1000 * (1 << i));
    }
    return -1;
}

bool UdpTransport::WaitForAck(uint32_t seq_num) {
    using namespace std::chrono;
    
    // ACK等待超时时间设置
    const auto timeout = milliseconds(500);  // 500ms超时
    auto start_time = steady_clock::now();
    
    uint8_t buf[256];  // 接收缓冲区
    sockaddr_in src_addr;
    
    while (steady_clock::now() - start_time < timeout) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockfd_, &read_fds);
        
        // 计算剩余等待时间
        auto elapsed = steady_clock::now() - start_time;
        auto remaining = timeout - duration_cast<milliseconds>(elapsed);
        if (remaining <= milliseconds::zero()) {
            return false;  // 超时
        }
        
        // 设置select超时
        struct timeval tv;
        tv.tv_sec = remaining.count() / 1000;
        tv.tv_usec = (remaining.count() % 1000) * 1000;
        
        // 等待数据可读
        int ret = select(sockfd_ + 1, &read_fds, nullptr, nullptr, &tv);
        if (ret < 0) {
            if (errno == EINTR) continue;  // 被信号中断，继续等待
            LOG_ERROR("Select failed: %s", strerror(errno));
            return false;
        }
        if (ret == 0) continue;  // 超时，继续等待
        
        // 接收数据
        ssize_t recv_len = RecvFrom(buf, sizeof(buf), &src_addr);
        if (recv_len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            LOG_ERROR("Receive failed: %s", strerror(errno));
            return false;
        }
        
        // 验证ACK包
        if (recv_len >= sizeof(uint32_t)) {
            uint32_t recv_seq = ntohl(*reinterpret_cast<uint32_t*>(buf));
            if (recv_seq == seq_num) {
                return true;  // 收到正确的ACK
            }
        }
    }
    
    return false;  // 超时未收到正确的ACK
}