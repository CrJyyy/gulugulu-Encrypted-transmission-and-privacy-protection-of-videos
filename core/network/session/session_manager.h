//会话管理类声明

#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "session_context.h"
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>
#include <netinet/in.h>
#include <memory>

class SessionManager {
public:
    static SessionManager& GetInstance() {
    static SessionManager instance;
    return instance;
    }

    //删除复制构造和赋值操作
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;
    
    //创建新会话 (返回session_id)
    uint32_t CreateSession(const struct sockaddr_in& client_addr);
    
    //获取会话上下文
    SessionContext* GetSession(uint32_t session_id);
    
    // 清理超时会话
    void CleanupInactiveSessions(uint64_t timeout_ms = 30000);

    //检查会话是否存在
    bool HasSession(uint32_t session_id) const;

    //移除指定会话
    bool RemoveSession(uint32_t session_id);

private:
    //私有构造函数
    SessionManager() : next_session_id_(1), last_cleanup_(std::chrono::steady_clock::now()) {}
    
    //生成唯一会话ID
    uint32_t GenerateSessionId() {
        return next_session_id_.fetch_add(1, std::memory_order_relaxed);
    }

    std::unordered_map<uint32_t, std::unique_ptr<SessionContext>> sessions_;
    std::mutex mutex_;
    std::atomic<uint32_t> next_session_id_;
    std::chrono::steady_clock::time_point last_cleanup_;
};

#endif