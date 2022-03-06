#include <sys/epoll.h>
#include <exception>

#include "epoll.hpp"

namespace bstcp {

const size_t max_events = 10;
const size_t timeout    = 1000;

Epoll::Epoll()
    : _epoll_fd(epoll_create1(0)) {}

bool Epoll::delete_client(const std::shared_ptr<IServerClient>& client) {
    auto socket_fd = client->get_socket();
    if (_clients.find(socket_fd) == _clients.end()) {
        return false;
    }
    std::lock_guard lock(_mutex);
    _clients.erase(socket_fd);
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client->get_socket(), nullptr) == -1) {
        return false;
    }
    return true;
}

bool Epoll::_delete_ctl(socket_t socket) const {
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, socket, nullptr) == -1) {
        return false;
    }
    return true;
}

std::vector<Epoll::epoll_event_t> Epoll::wait() {
    if (!_serv_socket) {
        return {};
    }

    std::vector<struct epoll_event> events(max_events);
    std::lock_guard lock(_mutex);
    auto number = epoll_wait(_epoll_fd, events.data(), max_events, timeout);
    std::vector<epoll_event_t> selected;

    for (int i = 0; i < number; ++i) {
        epoll_event_t epollEvent;

        if (events[i].data.fd != _serv_socket->get_socket()
        && _clients.find(events[i].data.fd) == _clients.end()) {
            if (!_delete_ctl(events[i].data.fd)) {
                throw std::runtime_error("Some strange");
            }
            continue;
        }

        if (events[i].events & EPOLLHUP || events[i].events & EPOLLRDHUP) {
            epollEvent.event = event_t::close;
        } else if (events[i].events & EPOLLIN) {
             if(events[i].data.fd == _serv_socket->get_socket()) {
                 epollEvent.event = event_t::need_accept;
                 selected.push_back(epollEvent);
                 continue;
             } else {
                 epollEvent.event = event_t::can_read;
             }
         } else {
            epollEvent.event = event_t::err;
        }
        epollEvent.client = _clients[events[i].data.fd];

        selected.push_back(epollEvent);
    }

    return selected;
}

bool Epoll::add_client(std::unique_ptr<IServerClient>&& client) {
    struct epoll_event ev{};
    auto socket_fd = client->get_socket();
    ev.data.fd = socket_fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    {
        std::lock_guard lock(_mutex);
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client->get_socket(), &ev) ==
            -1) {
            return false;
        }
        _clients[socket_fd] = Client(
                std::shared_ptr<IServerClient>(client.release()));
    }
    return true;
}

bool Epoll::add_server_socket(std::unique_ptr<ISocket> server) {
    _serv_socket = std::move(server);
    struct epoll_event ev{};
    ev.data.fd = _serv_socket->get_socket();
    ev.events = EPOLLIN;

    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _serv_socket->get_socket(), &ev) == -1) {
        return false;
    }
    return true;
}

void Epoll::stop() {
    std::lock_guard lock(_mutex);
    _serv_socket->disconnect();
    _serv_socket = nullptr;

    for(auto& client : _clients) {
        _delete_ctl(client.second.get_client()->get_socket());
    }

    _clients.clear();
}

std::vector<Epoll::Client> Epoll::get_clients() {
    std::vector<Client> res;
    for (const auto& client: _clients) {
        res.push_back(client.second);
    }
    return res;
}

void Epoll::delete_all() {
    std::lock_guard lock(_mutex);
    for(auto& client : _clients) {
        _delete_ctl(client.second.get_client()->get_socket());
    }

    _clients.clear();
}

Epoll::Client::Client(std::shared_ptr<IServerClient>&& client)
    : _access_mtx(new std::mutex())
    , _client(std::move(client)){}

bool Epoll::Client::try_lock() const {
    return _access_mtx->try_lock();
}

void Epoll::Client::lock() const {
    _access_mtx->lock();
}

const std::shared_ptr<IServerClient>& Epoll::Client::get_client() const{
    return _client;
}

void Epoll::Client::unlock() const {
    _access_mtx->unlock();
}

Epoll::Client::Client()
    : _access_mtx(new std::mutex())
    , _client(){}

const std::unique_ptr<ISocket> &Epoll::get_server() const {
    return _serv_socket;
}

}