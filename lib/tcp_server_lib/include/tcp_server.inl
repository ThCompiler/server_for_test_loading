#include <chrono>
#include <cstring>
#include <mutex>
#include <utility>

using namespace bstcp;

SOCKET_TEMPLATE
TcpServer<Socket, T>::TcpServer(uint16_t port,
                                KeepAliveConfig ka_conf,
                                _con_handler_function_t connect_hndl,
                                _con_handler_function_t disconnect_hndl,
                                size_t thread_count
)
        : _port(port)
          , _thread_pool()
          , _ka_conf(ka_conf)
          , _connect_hndl(std::move(connect_hndl))
          , _disconnect_hndl(std::move(disconnect_hndl)) {
    _thread_pool.set_max_threads(thread_count);
}

SOCKET_TEMPLATE
TcpServer<Socket, T>::~TcpServer() {
    if (_status == ServerStatus::up) {
        stop();
    }
}

SOCKET_TEMPLATE
uint16_t TcpServer<Socket, T>::get_port() const {
    return _port;
}

SOCKET_TEMPLATE
uint16_t TcpServer<Socket, T>::set_port(uint16_t port) {
    _port = port;
    start();
    return port;
}

SOCKET_TEMPLATE
typename bstcp::TcpServer<Socket, T>::ServerStatus
TcpServer<Socket, T>::start() {
    if (_status == ServerStatus::up) {
        stop();
    }

    uniq_ptr<Socket> serv_socket(new Socket());
    auto sts = serv_socket->init(localhost, _port,
                                 (uint16_t) SocketType::nonblocking_socket
                                 | (uint16_t) SocketType::server_socket);
    switch (sts) {
        case SocketStatus::connected:
            break;
        case SocketStatus::err_socket_bind:
            return _status = ServerStatus::err_socket_bind;
        case SocketStatus::err_socket_init:
            return _status = ServerStatus::err_socket_init;
        case SocketStatus::err_socket_listening:
            return _status = ServerStatus::err_socket_listening;
        case SocketStatus::err_socket_connect:
            return _status = ServerStatus::close;
        case SocketStatus::disconnected:
            return _status = ServerStatus::close;
        case SocketStatus::err_socket_type:
            return _status = ServerStatus::close;
        default:
            return _status = ServerStatus::close;
    }
    _epoll.add_server_socket(std::move(serv_socket));

    _status = ServerStatus::up;
    _thread_pool.add([this] { _waiting_recv_loop(); });

    return _status;
}

SOCKET_TEMPLATE
void TcpServer<Socket, T>::stop() {
    _thread_pool.wait();
    _status = ServerStatus::close;

    _epoll.stop();
}

SOCKET_TEMPLATE
void TcpServer<Socket, T>::joinLoop() {
    _thread_pool.join();
}

SOCKET_TEMPLATE
bool TcpServer<Socket, T>::connect_to(uint32_t host, uint16_t port,
                                      const _con_handler_function_t &connect_hndl) {
    uniq_ptr<Socket> client_socket;
    auto sts = client_socket->init(host, port,
                                  (uint16_t) SocketType::nonblocking_socket
                                  | (uint16_t) SocketType::client_socket);
    if (sts != status::connected) {
        return false;
    }

    if (!_enable_keep_alive(client_socket.get_socket())) {
        client_socket->disconnect();
        return false;
    }

    connect_hndl(client_socket);
    _epoll_mutex.lock();
    _epoll.add_client(uniq_ptr<T>(std::move(client_socket)));
    _epoll_mutex.unlock();
    return true;
}

SOCKET_TEMPLATE
void TcpServer<Socket, T>::disconnect_all() {
    _epoll.delete_all();
}

SOCKET_TEMPLATE
void TcpServer<Socket, T>::_accept_loop(std::shared_ptr<ISocket> server) {
    Socket client_socket;
    if (client_socket.accept(server) == status::connected
        && _status == ServerStatus::up) {

        if (_enable_keep_alive(client_socket.get_socket())) {
            uniq_ptr<T> client(new T(std::move(client_socket)));
            _connect_hndl(reinterpret_cast<std::unique_ptr<IServerClient> &>(client));
            _epoll_mutex.lock();
            _epoll.add_client(uniq_ptr<T>(std::move(client)));
            _epoll_mutex.unlock();
        }
    }
}

SOCKET_TEMPLATE
void TcpServer<Socket, T>::_waiting_recv_loop() {
    {
        std::lock_guard lock(_epoll_mutex);
        auto res = _epoll.wait();
        for (auto client : res) {
            switch (client.event) {
                case Epoll::err:
                case Epoll::event_t::close:
                    _epoll.delete_client(client.client.get_client());
                    _thread_pool.add([this, &client] {
                        client.client.lock();
                        _disconnect_hndl(
                                reinterpret_cast<std::unique_ptr<IServerClient> &>(client.client.get_client()));
                        client.client.get_client()->disconnect();
                        client.client.unlock();
                    });
                    break;
                case Epoll::can_read:
                    _thread_pool.add([this, &client] {
                        client.client.lock();
                        _accept_loop(client.client.get_client());
                        client.client.unlock();
                    });
                    break;
                case Epoll::need_accept:
                    _thread_pool.add(
                        [this, &client] {
                            client.client.lock();
                            if (client.client.get_client()->get_status() !=
                                SocketStatus::disconnected) {
                                client.client.get_client()->handle_request();
                            }
                            client.client.unlock();
                        });
                    break;
            }
        }
    }

    if (_status == ServerStatus::up) {
        _thread_pool.add([this]() { _waiting_recv_loop(); });
    }
}

SOCKET_TEMPLATE
bool TcpServer<Socket, T>::_enable_keep_alive(socket_t socket) {
    int flag = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag)) ==
        -1) {
        return false;
    }
    if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &_ka_conf.ka_idle,
                   sizeof(_ka_conf.ka_idle)) == -1) {
        return false;
    }
    if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL, &_ka_conf.ka_intvl,
                   sizeof(_ka_conf.ka_intvl)) == -1) {
        return false;
    }
    if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &_ka_conf.ka_cnt,
                   sizeof(_ka_conf.ka_cnt)) == -1) {
        return false;
    }
    return true;
}

SOCKET_TEMPLATE
typename TcpServer<Socket, T>::ServerStatus
TcpServer<Socket, T>::get_status() const {
    return _status;
}

SOCKET_TEMPLATE
prll::Parallel &TcpServer<Socket, T>::get_thread_pool() {
    return _thread_pool;
}
