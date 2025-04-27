#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <string>

class Proxy : public std::enable_shared_from_this<Proxy> {
public:
    Proxy(boost::asio::ip::tcp::socket socket, const std::string& name);
    void start();
    void update(const std::string& msg, int logical_time, const std::string& producer);

private:
    void do_read();
    void do_write(const std::string& msg);

    boost::asio::ip::tcp::socket socket_;
    std::string name_;
    boost::asio::streambuf buffer_;
};
