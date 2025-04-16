//会话管理实现

#include <iostream>
#include "session_manager.h"
#include <chrono>
using namespace std;

uint32_t SessionManager::CreateSession(const struct sockaddr_in& client_addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint32_t session_id = GenerateSessionId();
    
    auto session = std::make_unique<SessionContext>(session_id, client_addr);
    sessions_[session_id] = std::move(session);
    
    return session_id;
}

SessionContext* SessionManager::GetSession(uint32_t session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        if (it->second->IsValid()) {  // 检查会话是否有效
            it->second->UpdateLastActive();  // 更新最后活跃时间
            return it->second.get();
        } else {
            sessions_.erase(it);  // 移除无效会话
            return nullptr;
        }
    }
    return nullptr;
}



void SessionManager::CleanupInactiveSessions(uint64_t timeout_ms) {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto it = sessions_.begin(); it != sessions_.end();) {
        // 使用 SessionContext 的 GetLastActive 方法
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - it->second->GetLastActive());
            
        if (elapsed.count() > timeout_ms) {
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

void SessionContext::AdjustCongestionWindow(bool ack_received) {
    if (ack_received) {
        congestion_window = std::min(congestion_window * 2, max_window_size);
    } else {
        congestion_window = std::max(congestion_window / 2, min_window_size);
    }
}