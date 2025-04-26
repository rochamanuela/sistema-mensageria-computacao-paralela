#include <iostream>
#include <fstream>
#include <deque>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <map>
#include <set>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

struct Message {
    string content;
    string producer;
    int logical_time;
};

class Proxy; // Forward declaration

class MsgQMgr {
public:
    static MsgQMgr* getInstance() {
        static MsgQMgr instance;
        return &instance;
    }

    void start() {
        logical_time_ = 0;
    }

    void addClient(const string& name, shared_ptr<Proxy> proxy) {
        lock_guard<mutex> lock(clients_mutex_);
        clients_[name] = proxy;
    }

    void removeClient(const string& name) {
        lock_guard<mutex> lock(clients_mutex_);
        clients_.erase(name);
    }

    void addMsg(const string& msg, const string& producer) {
        lock_guard<mutex> lock(msg_q_mutex_);
        logical_time_++;
        messages_.push_back({ msg, producer, logical_time_ });
        logEvent("[ENVIADO] Produtor: " + producer + ", Msg: " + msg + ", Tempo: " + to_string(logical_time_));
        condition_.notify_all();
    }

    void broadcastMsg(const string& msg, const string& producer) {
        addMsg(msg, producer);
        lock_guard<mutex> lock(clients_mutex_);
        for (auto& [name, client] : clients_) {
            client->update(msg, logical_time_, producer);
        }
    }

    void multicastMsg(const string& msg, const string& producer, const set<string>& recipients) {
        addMsg(msg, producer);
        lock_guard<mutex> lock(clients_mutex_);
        for (const auto& name : recipients) {
            if (clients_.count(name))
                clients_[name]->update(msg, logical_time_, producer);
        }
    }

    void unicastMsg(const string& msg, const string& producer, const string& recipient) {
        addMsg(msg, producer);
        lock_guard<mutex> lock(clients_mutex_);
        if (clients_.count(recipient))
            clients_[recipient]->update(msg, logical_time_, producer);
    }

    void logEvent(const string& log) {
        ofstream logFile("log.txt", ios::app);
        logFile << log << endl;
    }

private:
    MsgQMgr() {}
    int logical_time_;
    deque<Message> messages_;
    mutex msg_q_mutex_;
    map<string, shared_ptr<Proxy>> clients_;
    mutex clients_mutex_;
    condition_variable condition_;
};

class Proxy : public enable_shared_from_this<Proxy> {
public:
    Proxy(tcp::socket socket)
        : socket_(move(socket)) {}

    void start() {
        doRead();
    }

    void update(const string& msg, int logical_time, const string& producer) {
        string full_msg = "[RECEBIDO] Consumidor: " + name_ + ", Produtor: " + producer + ", Msg: " + msg + ", Tempo: " + to_string(logical_time);
        MsgQMgr::getInstance()->logEvent(full_msg);
        boost::asio::async_write(socket_, boost::asio::buffer(full_msg + "\n"),
            [](boost::system::error_code, size_t) {});
    }

private:
    void doRead() {
        auto self = shared_from_this();
        boost::asio::async_read_until(socket_, buffer_, '\n',
            [this, self](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {
                    istream is(&buffer_);
                    string line;
                    getline(is, line);
                    if (!line.empty()) {
                        handleCommand(line);
                    }
                    doRead();
                } else {
                    MsgQMgr::getInstance()->removeClient(name_);
                }
            });
    }

    void handleCommand(const string& line) {
        size_t sep = line.find('|');
        if (sep == string::npos) return;

        string producer = line.substr(0, sep);
        string rest = line.substr(sep + 1);

        if (name_.empty()) {
            name_ = producer;
            MsgQMgr::getInstance()->addClient(name_, shared_from_this());
        }

        istringstream iss(rest);
        string cmd;
        iss >> cmd;

        if (cmd == "broadcast") {
            string msg;
            getline(iss, msg);
            MsgQMgr::getInstance()->broadcastMsg(trim(msg), producer);
        } else if (cmd == "unicast") {
            string recipient, msg;
            iss >> recipient;
            getline(iss, msg);
            MsgQMgr::getInstance()->unicastMsg(trim(msg), producer, recipient);
        } else if (cmd == "multicast") {
            string recipients_str, msg;
            iss >> recipients_str;
            getline(iss, msg);

            set<string> recipients;
            size_t pos = 0;
            while ((pos = recipients_str.find(',')) != string::npos) {
                recipients.insert(recipients_str.substr(0, pos));
                recipients_str.erase(0, pos + 1);
            }
            recipients.insert(recipients_str);
            MsgQMgr::getInstance()->multicastMsg(trim(msg), producer, recipients);
        }
    }

    string trim(const string& s) {
        size_t start = s.find_first_not_of(" ");
        size_t end = s.find_last_not_of(" ");
        return (start == string::npos || end == string::npos) ? "" : s.substr(start, end - start + 1);
    }

    tcp::socket socket_;
    string name_;
    boost::asio::streambuf buffer_;
};

class Server {
public:
    Server(boost::asio::io_service& io_service, short port)
        : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
          socket_(io_service) {
        doAccept();
    }

private:
    void doAccept() {
        acceptor_.async_accept(socket_,
            [this](boost::system::error_code ec) {
                if (!ec) {
                    make_shared<Proxy>(move(socket_))->start();
                }
                doAccept();
            });
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
};

int main() {
    try {
        boost::asio::io_service io_service;
        MsgQMgr::getInstance()->start();
        Server server(io_service, 12345);
        cout << "Servidor iniciado na porta 12345...\n";
        io_service.run();
    } catch (exception& e) {
        cerr << "Erro no servidor: " << e.what() << endl;
    }
    return 0;
}
