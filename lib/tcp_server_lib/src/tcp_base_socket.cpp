#include "tcp_base_socket.hpp"

using namespace bstcp;

#include <iostream>

BaseSocket::~BaseSocket() {
    _status = status::disconnected;
    if (_socket == -1) {
        return;
    }

    shutdown(_socket, SD_BOTH);

    close(_socket);
    _socket = -1;
}

status BaseSocket::init(uint32_t host, uint16_t port, uint16_t type) {
    if (_status == status::connected) {
        disconnect();
    }

    if (type & (uint16_t)SocketType::server_socket) {
        return _init_as_server(host, port, type);
    }
    if (type & (uint16_t)SocketType::client_socket) {
        return _init_as_client(host, port, type);
    }
    return status::err_socket_type;
}

status BaseSocket::_init_as_client(uint32_t host, uint16_t port, uint16_t type) {
    if ((_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0) {
        return _status = status::err_socket_init;
    }

    new(&_address) socket_addr_in;
    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = host;
    _address.sin_port = htons(port);


    if (connect(_socket, (sockaddr *) &_address, sizeof(_address)) != 0) {
        close(_socket);
        return _status = status::err_socket_connect;
    }

    int flags = fcntl(_socket, F_GETFL, 0);
    if (flags == -1) {
        close(_socket);
        return _status = status::err_socket_init;
    }
    flags = (type & (uint16_t)SocketType::nonblocking_socket) ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    if (fcntl(_socket, F_SETFL, flags) != 0) {
        close(_socket);
        return _status = status::err_socket_init;
    }

    return _status = status::connected;
}

status BaseSocket::accept(const std::unique_ptr<ISocket>& server_socket) {
    if (_status == status::connected) {
        disconnect();
    }

    sock_len_t addrlen = sizeof(socket_addr_in);
    if ((_socket = accept4(server_socket->get_socket(), (struct sockaddr *) &_address,
                           &addrlen, O_NONBLOCK)) < 0) {
        return _status = status::disconnected;
    }

    return _status = status::connected;
}

status BaseSocket::_init_as_server(uint32_t, uint16_t port, uint16_t type) {
    socket_addr_in address;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    address.sin_family = AF_INET;


    int type_ = SOCK_STREAM;
    if (type & (uint16_t)SocketType::nonblocking_socket) {
        type_ |= SOCK_NONBLOCK;
    }

    if ((_socket = socket(AF_INET, type_, 0)) == -1) {
        return _status = status::err_socket_init;
    }

    if (int flag = true; setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1)  {
        return _status = status::err_socket_bind;
    }

    if (bind(_socket, (struct sockaddr *) &address, sizeof(address)) < 0) {
        return _status = status::err_socket_bind;
    }

    if (listen(_socket, SOMAXCONN) < 0) {
        return _status = status::err_socket_listening;
    }

    return _status = status::connected;
}

bool BaseSocket::recv_from(void *buffer, int size) {
    if (_status != SocketStatus::connected)  {
        return false;
    }

    ssize_t answ = recv(_socket, reinterpret_cast<char *>(buffer), size, 0);

    if (answ <= 0 || !size) {
        return false;
    }

    return true;
}

bool BaseSocket::send_to(const void *buffer, int size) const {
    if (_status != SocketStatus::connected) {
        return false;
    }

    if (send(_socket, reinterpret_cast<const char *>(buffer), size, 0) < 0) {
        return false;
    }

    return true;
}

status BaseSocket::disconnect() {
    _status = status::disconnected;

    if (_socket == -1) {
        return _status;
    }

    shutdown(_socket, SD_BOTH);
    close(_socket);
    _socket = -1;

    return _status;
}

uint32_t BaseSocket::get_host() const {
    return _address.sin_addr.s_addr;
}

uint16_t BaseSocket::get_port() const {
    return _address.sin_port;
}

BaseSocket &BaseSocket::operator=(BaseSocket &&sok) noexcept {
    _status     = sok._status;
    _socket     = sok._socket;
    _type       = sok._type;
    _address    = sok._address;

    sok._socket     = -1;
    sok._status     = status::disconnected;
    sok._address    = socket_addr_in();
    sok._type       = (uint16_t)SocketType::unset_type;
    return *this;
}

BaseSocket::BaseSocket(BaseSocket &&sok) noexcept
        : _status   (sok._status)
          , _type   (sok._type)
          , _socket (sok._socket)
          , _address(sok._address) {

    sok._socket     = -1;
    sok._status     = status::disconnected;
    sok._address    = socket_addr_in();
    sok._type       = (uint16_t)SocketType::unset_type;
}

bool BaseSocket::is_allow_to_read(long timeout) const {
    if (_status != status::connected) {
        return false;
    }

    fd_set rfds;
    struct timeval tv{};
    FD_ZERO(&rfds);
    FD_SET(_socket, &rfds);

    tv.tv_sec = timeout / 1000;
    tv.tv_usec = timeout % 1000;
    int selres = select(_socket + 1, &rfds, nullptr, nullptr, &tv);
    switch (selres){
        case -1:
            return false;
        case 0:
            return false;
        default:
            break;
    }

    if (FD_ISSET(_socket, &rfds)){
        return true;
    } else {
        return false;
    }
}

bool BaseSocket::is_allow_to_write(long timeout) const {
    if (_status != status::connected) {
        return false;
    }

    fd_set wfds;
    struct timeval tv{};
    FD_ZERO(&wfds);
    FD_SET(_socket, &wfds);

    tv.tv_sec = timeout / 1000;
    tv.tv_usec = timeout % 1000;
    int selres = select(_socket + 1, nullptr, &wfds, nullptr, &tv);
    switch (selres){
        case -1:
            return false;
        case 0:
            return false;
        default:
            break;
    }

    if (FD_ISSET(_socket, &wfds)){
        return true;
    } else {
        return false;
    }
}

bool BaseSocket::is_allow_to_rwrite(long timeout) const {
    if (_status != status::connected) {
        return false;
    }

    fd_set rfds, wfds;
    struct timeval tv{};
    FD_ZERO(&wfds);
    FD_SET(_socket, &wfds);
    FD_ZERO(&rfds);
    FD_SET(_socket, &rfds);

    tv.tv_sec = timeout / 1000;
    tv.tv_usec = timeout % 1000;
    int selres = select(_socket + 1,  &rfds, &wfds, nullptr, &tv);
    switch (selres){
        case -1:
            return false;
        case 0:
            return false;
        default:
            break;
    }

    if (FD_ISSET(_socket, &rfds) && FD_ISSET(_socket, &wfds)){
        return true;
    } else {
        return false;
    }
}

socket_t BaseSocket::get_socket() {
    return _socket;
}

socket_addr_in BaseSocket::get_address() const {
    return _address;
}

status BaseSocket::get_status() const {
    return _status;
}

SocketType BaseSocket::get_type() const {
    return (SocketType)_type;
}
