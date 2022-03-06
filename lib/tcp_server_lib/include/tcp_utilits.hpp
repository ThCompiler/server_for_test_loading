#pragma once

#define SD_BOTH 0
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>


#include <cstdint>
#include <cstring>
#include <cinttypes>
#include <malloc.h>

#include <queue>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace bstcp {

constexpr uint32_t localhost = 0x0100007f; //127.0.0.1

typedef socklen_t sock_len_t;
typedef struct sockaddr_in socket_addr_in;
typedef int socket_t;
typedef int ka_prop_t;

template <typename T>
using uniq_ptr = std::unique_ptr<T>;

enum class SocketType : uint16_t {
    unset_type          = 0,
    client_socket       = 1,
    server_socket       = 2,
    blocking_socket     = 4,
    nonblocking_socket  = 8,
};

enum class SocketStatus : uint8_t {
    connected               = 0,
    err_socket_init         = 1,
    err_socket_bind         = 2,
    err_socket_connect      = 3,
    disconnected            = 4,
    err_socket_type         = 5,
    err_socket_listening    = 6,
};

int hostname_to_ip(const char *hostname, socket_addr_in *addr);

typedef std::vector<uint8_t> tcp_data_t;

typedef SocketStatus status;

class IReceivable {
  public:
    virtual ~IReceivable() = default;

    virtual bool recv_from(void *buffer, int size) = 0;
};

class ISendable {
  public:
    virtual ~ISendable() = default;

    virtual bool send_to(const void *buffer, int size) const = 0;
};

class ISendRecvable : public IReceivable, public ISendable {
  public:
    ~ISendRecvable() override = default;
};

class IDisconnectable {
  public:
    virtual ~IDisconnectable() = default;

    virtual status disconnect() = 0;
};

class ISocket : public ISendRecvable, public IDisconnectable {
  public:
    ~ISocket() override = default;

    virtual status accept(const std::unique_ptr<ISocket>& server_socket) = 0;

    [[nodiscard]] virtual status get_status() const = 0;

    [[nodiscard]] virtual socket_t get_socket() = 0;

    [[nodiscard]] virtual uint32_t get_host() const = 0;

    [[nodiscard]] virtual uint16_t get_port() const = 0;

    [[nodiscard]] virtual SocketType get_type() const = 0;

    [[nodiscard]] virtual bool is_allow_to_read(long timeout) const = 0;

    [[nodiscard]] virtual bool is_allow_to_write(long timeout) const = 0;

    [[nodiscard]] virtual  bool is_allow_to_rwrite(long timeout) const = 0;
};

}