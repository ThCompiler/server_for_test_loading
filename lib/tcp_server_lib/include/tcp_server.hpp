#pragma once

#include <functional>
#include <list>

#include <thread>
#include <mutex>
#include <shared_mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>

#include "concepts.hpp"
#include "parallel.hpp"
#include "epoll.hpp"

namespace bstcp {

struct KeepAliveConfig {
    ka_prop_t ka_idle = 120;
    ka_prop_t ka_intvl = 3;
    ka_prop_t ka_cnt = 5;
};

SOCKET_TEMPLATE
class TcpServer {
public:
    enum class ServerStatus : uint8_t {
        up                      = 0,
        err_socket_init         = 1,
        err_socket_bind         = 2,
        err_scoket_keep_alive   = 3,
        err_socket_listening    = 4,
        close                   = 5
    };

    typedef std::function<void(const std::unique_ptr<IServerClient> &)>   _con_handler_function_t;

    static constexpr auto _default_connsection_handler  = [](const std::unique_ptr<IServerClient> &) {};

  public:
    explicit TcpServer(uint16_t port,
                       KeepAliveConfig ka_conf = {},
                       _con_handler_function_t connect_hndl = _default_connsection_handler,
                       _con_handler_function_t disconnect_hndl = _default_connsection_handler,
                       size_t thread_count = std::thread::hardware_concurrency()
    );

    ~TcpServer();

    prll::Parallel &get_thread_pool();

    [[nodiscard]] uint16_t get_port() const;

    uint16_t set_port(uint16_t port);

    [[nodiscard]] ServerStatus get_status() const;

    ServerStatus start();

    void stop();

    void joinLoop();

    // Server client management
    bool connect_to(uint32_t host, uint16_t port,
                   const _con_handler_function_t& connect_hndl);

    void disconnect_all();

  private:
    Epoll           _epoll;
    uint16_t        _port;
    std::mutex      _epoll_mutex;
    ServerStatus    _status  = ServerStatus::close;
    prll::Parallel  _thread_pool;
    KeepAliveConfig _ka_conf;

    _con_handler_function_t _connect_hndl       = _default_connsection_handler;
    _con_handler_function_t _disconnect_hndl    = _default_connsection_handler;

    bool _enable_keep_alive(socket_t socket);

    void _accept_loop(const std::unique_ptr<ISocket> &server);

    void _waiting_recv_loop();
};


#if __cplusplus > 201703L && __cpp_concepts >= 201907L
template<server_client<BaseSocket> T>
using BaseTcpServer = TcpServer<BaseSocket, T>;
#else

template<class T,typename = std::enable_if<server_client<T, BaseSocket>::value>>
using BaseTcpServer = TcpServer<BaseSocket, T>;

#endif

}

#include "tcp_server.inl"