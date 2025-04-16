//会话上下文定义

#ifndef SESSION_CONTEXT_H
#define SESSION_CONTEXT_H

#include <sys/socket.h>
#include <chrono>
#include <vector>
#include <netinet/in.h> 
#include <atomic>

class SessionContext {
public:
    SessionContext(uint32_t id, const sockaddr_in& addr) 
        : session_id(id), client_addr(addr), 
          last_active(std::chrono::steady_clock::now()),
          next_seq(0),
          congestion_window(1),  // 初始拥塞窗口大小
          rtt(0) {}             // 初始RTT

    uint32_t GetAndIncrementSeq() {
        return next_seq.fetch_add(1, std::memory_order_relaxed);
    }

    void UpdateLastActive() { 
        last_active = std::chrono::steady_clock::now(); 
    }

    bool IsValid() const {
        auto now = std::chrono::steady_clock::now();
        return (now - last_active) <= std::chrono::seconds(30);
    }

    std::chrono::steady_clock::time_point GetLastActive() const { 
        return last_active; 
    }

    void AdjustCongestionWindow(bool ack_received);

private:
    uint32_t session_id;
    sockaddr_in client_addr;
    uint8_t sm4_key[16];
    std::chrono::steady_clock::time_point last_active;
    std::atomic<uint32_t> next_seq;
    uint32_t congestion_window;
    uint32_t rtt;

    // 静态常量定义
    static const uint32_t max_window_size = 65535;  // 最大窗口大小
    static const uint32_t min_window_size = 1;      // 最小窗口大小
};

#endif