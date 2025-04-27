#pragma once
#include <unordered_map>
#include <memory>
#include <set>
#include <fstream>
#include <mutex>
#include <string>
#include "proxy.hpp"  // Inclui definição completa de Proxy!

class MsgQMgr {
public:
    static MsgQMgr* getInstance();

    void registerClient(const std::string& name, std::shared_ptr<Proxy> client);
    void handleMessage(const std::string& producer, const std::string& message);

    void start();

private:
    MsgQMgr();
    void log(const std::string& entry);

    void unicastMsg(const std::string& msg, const std::string& producer, const std::string& recipient);
    void multicastMsg(const std::string& msg, const std::string& producer, const std::set<std::string>& recipients);
    void broadcastMsg(const std::string& msg, const std::string& producer);

    std::unordered_map<std::string, std::shared_ptr<Proxy>> clients_;
    int logical_time_;
    std::mutex mtx_;
    std::ofstream logfile_;
};
