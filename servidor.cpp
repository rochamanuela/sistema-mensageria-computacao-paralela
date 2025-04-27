#include "servidor.hpp"

MsgQMgr* MsgQMgr::getInstance() {
    static MsgQMgr instance;
    return &instance;
}

MsgQMgr::MsgQMgr() : logical_time_(0) {
    logfile_.open("log.txt", std::ios::app);
}

void MsgQMgr::start() {
    // Nada aqui por enquanto
}

void MsgQMgr::registerClient(const std::string& name, std::shared_ptr<Proxy> client) {
    std::lock_guard<std::mutex> lock(mtx_);
    clients_[name] = client;
}

void MsgQMgr::handleMessage(const std::string& producer, const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx_);
    logical_time_++;

    log("RECEBIDO de " + producer + ": " + message + " @ " + std::to_string(logical_time_));

    if (message.starts_with("unicast:")) {
        auto rest = message.substr(8);
        auto pos = rest.find(':');
        if (pos != std::string::npos) {
            auto recipient = rest.substr(0, pos);
            auto msg = rest.substr(pos + 1);
            unicastMsg(msg, producer, recipient);
        }
    } else if (message.starts_with("multicast:")) {
        auto rest = message.substr(10);
        auto pos = rest.find(':');
        if (pos != std::string::npos) {
            auto recips = rest.substr(0, pos);
            auto msg = rest.substr(pos + 1);

            std::set<std::string> recipients;
            size_t start = 0;
            size_t end;
            while ((end = recips.find(',', start)) != std::string::npos) {
                recipients.insert(recips.substr(start, end - start));
                start = end + 1;
            }
            recipients.insert(recips.substr(start));

            multicastMsg(msg, producer, recipients);
        }
    } else if (message.starts_with("broadcast:")) {
        auto msg = message.substr(10);
        broadcastMsg(msg, producer);
    }
}

void MsgQMgr::log(const std::string& entry) {
    logfile_ << entry << std::endl;
}

void MsgQMgr::unicastMsg(const std::string& msg, const std::string& producer, const std::string& recipient) {
    if (clients_.count(recipient)) {
        clients_[recipient]->update(msg, logical_time_, producer);
        log("UNICAST para " + recipient + ": " + msg + " @ " + std::to_string(logical_time_));
    }
}

void MsgQMgr::multicastMsg(const std::string& msg, const std::string& producer, const std::set<std::string>& recipients) {
    for (auto& name : recipients) {
        if (clients_.count(name)) {
            clients_[name]->update(msg, logical_time_, producer);
        }
    }
    log("MULTICAST para vários: " + msg + " @ " + std::to_string(logical_time_));
}

void MsgQMgr::broadcastMsg(const std::string& msg, const std::string& producer) {
    for (auto& [name, client] : clients_) {
        client->update(msg, logical_time_, producer);
    }
    log("BROADCAST para todos: " + msg + " @ " + std::to_string(logical_time_));
}
