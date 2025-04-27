#include "proxy.hpp"
#include "servidor.hpp" // para poder chamar MsgQMgr::getInstance()

using boost::asio::ip::tcp;
using namespace std;

Proxy::Proxy(tcp::socket socket, const std::string& name)
    : socket_(std::move(socket)), name_(name) {}

void Proxy::start() {
    do_read();
}

void Proxy::update(const std::string& msg, int logical_time, const std::string& producer) {
    string decorated_msg = "From " + producer + " @ " + to_string(logical_time) + ": " + msg + "\n";
    do_write(decorated_msg);
}

void Proxy::do_read() {
    auto self(shared_from_this());
    boost::asio::async_read_until(socket_, buffer_, "\n",
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                istream is(&buffer_);
                string msg;
                getline(is, msg);

                MsgQMgr::getInstance()->handleMessage(name_, msg);

                do_read();
            }
        });
}

void Proxy::do_write(const std::string& msg) {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(msg),
        [this, self](boost::system::error_code /ec/, std::size_t /length/) {
            // Pode adicionar algo aqui se quiser
        });
}
