#include "tcp_server_lib.hpp"
#include "file_client_lib.hpp"

#include <iostream>
#include <getopt.h>

using namespace bstcp;


//Parse ip to std::string
std::string getHostStr(const std::unique_ptr<IServerClient> &client) {
    uint32_t ip = client->get_host();
    return std::string() + std::to_string(int(reinterpret_cast<unsigned char *>(&ip)[0])) + '.' +
           std::to_string(int(reinterpret_cast<unsigned char *>(&ip)[1])) + '.' +
           std::to_string(int(reinterpret_cast<unsigned char *>(&ip)[2])) + '.' +
           std::to_string(int(reinterpret_cast<unsigned char *>(&ip)[3])) + ':' +
           std::to_string(client->get_port());
}

int main(int argc, char *argv[]) {
    int opt = 0;

    int http_port = 8081;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        if (opt == 'p') {
            http_port = (int)strtol(optarg, nullptr, 10);
        }
    }

    try {
        BaseTcpServer<file::FileClient> server(http_port,
                         {1, 1, 1}, // Keep alive{idle:1s, interval: 1s, pk_count: 1}

                         [](const std::unique_ptr<IServerClient> &client) { // Connect handler
                             std::cout << "Client " << getHostStr(client) << " connected\n";
                         },

                         [](const std::unique_ptr<IServerClient> &client) { // Disconnect handler
                             std::cout << "Client " << getHostStr(client) << " disconnected\n";
                         },

                         20//std::thread::hardware_concurrency() // Thread pool size
        );

        //Start server
        if (server.start() == BaseTcpServer<file::FileClient>::ServerStatus::up) {
            std::cout << "Server listen on port: " << server.get_port() << std::endl
                      << "Server run on events: " << 20 << std::endl;
            server.joinLoop();
            return EXIT_SUCCESS;
        } else {
            std::cout << "Server start error! Error code:" << int(server.get_status()) << std::endl;
            return EXIT_FAILURE;
        }

    } catch (std::exception &except) {
        std::cerr << except.what();
        return EXIT_FAILURE;
    }

}