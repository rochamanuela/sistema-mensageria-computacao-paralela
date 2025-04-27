#include <iostream>
#include <boost/asio.hpp>
#include <memory>
#include "servidor.hpp" // substitua pelo nome do arquivo com MsgQMgr e Proxy

using boost::asio::ip::tcp;
using namespace std;

void do_accept(boost::asio::io_context& io_context, tcp::acceptor& acceptor) {
    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket) {
        if (!ec) {
            try {
                // Lê o nome do cliente
                boost::asio::streambuf buffer;
                boost::asio::read_until(socket, buffer, "\n");
                istream input(&buffer);
                string name;
                getline(input, name);

                cout << "[INFO] Cliente conectado: " << name << endl;

                // Cria uma instância de Proxy para este cliente
                auto proxy = make_shared<Proxy>(std::move(socket), name);
                proxy->start();
            } catch (std::exception& e) {
                cerr << "[ERRO] Durante leitura inicial: " << e.what() << endl;
            }
        }

        // Aceita a próxima conexão
        do_accept(io_context, acceptor);
    });
}

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345)); // porta 12345

        cout << "[SERVIDOR] Iniciando..." << endl;
        MsgQMgr::getInstance()->start();

        do_accept(io_context, acceptor);

        io_context.run();
    } catch (std::exception& e) {
        cerr << "[FATAL] Exceção: " << e.what() << endl;
    }

    return 0;
}
