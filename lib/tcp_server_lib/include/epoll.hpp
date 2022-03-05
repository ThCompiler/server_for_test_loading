#pragma once

#include <map>

#include "concepts.hpp"

namespace bstcp {

typedef int epoll_fd_t;

class Epoll {
  public:
    struct Client {
      public:
        Client() = default;

        explicit Client(std::shared_ptr<IServerClient> client);

        void lock();

        std::shared_ptr<IServerClient>& get_client();

        void unlock();

        ~Client() = default;

      private:
        std::shared_ptr<std::mutex>     _access_mtx;
        std::shared_ptr<IServerClient>  _client;
    };

    enum event_t: uint16_t {
        close       = 0,
        can_read    = 1,
        need_accept = 2,
        err         = 3
    };

    struct epoll_event_t {
        Client    client;
        event_t   event;
    };

    Epoll();

    bool add_server_socket(std::unique_ptr<ISocket> server);

    void stop();

    bool add_client(std::unique_ptr<IServerClient> client);

    std::vector<epoll_event_t> wait();

    bool delete_client(const std::shared_ptr<IServerClient>& client);

    std::vector<Client> get_clients();

    void delete_all();

  private:

    bool _delete_ctl(socket_t socket) const;

    std::map<size_t, Client> _clients;

    epoll_fd_t                  _epoll_fd;
    std::unique_ptr<ISocket>    _serv_socket;
};

}