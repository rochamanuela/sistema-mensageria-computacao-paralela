#include <iostream>
#include <thread>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
using namespace std;

class Client {
public:
    Client(boost::asio::io_service& io_service, const string& host, int port, const string& name)
        : socket_(io_service), name_(name) {
        tcp::resolver resolver(io_service);
        auto endpoint_iterator = resolver.resolve({ host, to_string(port) });
        boost::asio::connect(socket_, endpoint_iterator);
    }

    void start() {
        thread reader([this]() { readLoop(); });
        sendLoop();
        reader.join();
    }

private:
    void sendLoop() {
        cout << "Conectado como '" << name_ << "'. Comandos disponíveis:\n";
        cout << "[1] unicast <destinatario> <mensagem>\n";
        cout << "[2] multicast <dest1,dest2,...> <mensagem>\n";
        cout << "[3] broadcast <mensagem>\n";
        cout << "[4] sair\n";

        while (true) {
            cout << "> ";
            string command;
            getline(cin, command);

            if (command == "sair" || command == "4")
                break;

            string full_msg = name_ + "|" + command;
            boost::asio::write(socket_, boost::asio::buffer(full_msg + "\n"));
        }
    }

    void readLoop() {
        boost::asio::streambuf buffer;
        while (true) {
            try {
                boost::asio::read_until(socket_, buffer, "\n");
                istream is(&buffer);
                string line;
                getline(is, line);
                if (!line.empty()) {
                    cout << "\n[Servidor]: " << line << "\n> ";
                    cout.flush();
                }
            } catch (...) {
                cout << "\n[Desconectado do servidor]\n";
                break;
            }
        }
    }

    tcp::socket socket_;
    string name_;
};

int main() {
    string nome_cliente;
    cout << "Digite o nome do cliente: ";
    getline(cin, nome_cliente);

    try {
        boost::asio::io_service io_service;
        Client client(io_service, "127.0.0.1", 12345, nome_cliente); // Porta usada no servidor
        client.start();
    } catch (exception& e) {
        cerr << "Erro: " << e.what() << endl;
    }

    return 0;
}
