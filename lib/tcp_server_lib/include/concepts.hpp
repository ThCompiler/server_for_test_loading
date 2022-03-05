#pragma once

#include <concepts>
#include <type_traits>

#include "tcp_utilits.hpp"
#include "tcp_base_socket.hpp"

namespace bstcp {

class IServerClient: public ISocket {
  public:
    virtual void handle_request() = 0;

    ~IServerClient() override = default;
};

#if __cplusplus > 201703L && __cpp_concepts >= 201907L
template<typename T>
concept socket_type =   (std::is_base_of_v<ISocket, T>
                        || std::is_convertible_v<T, ISocket>)
                        && std::is_move_assignable_v<T>;

template<typename T, class Socket = BaseSocket>
concept server_client = std::is_constructible<T, Socket&&>::value
                        && std::is_destructible_v<T>
                        && (std::is_base_of_v<IServerClient, T>
                        || std::is_convertible_v<T, IServerClient>)
                        && std::is_move_constructible_v<T>;

#define SOCKET_TEMPLATE template<socket_type Socket, server_client<Socket> T>
#else
template<typename T>
using socket_type =  std::conjunction<
                        std::disjunction<std::is_base_of<ISocket, T>,
                                        std::is_convertible<T, ISocket>>,
                        std::is_move_assignable<T>>;

template<typename T, class Socket = BaseSocket>
using server_client = std::conjunction<std::is_constructible<T, Socket&&>,
                        std::is_destructible<T>,
                        std::disjunction<std::is_base_of<IServerClient, T>,
                            std::is_convertible<T, IServerClient>>,
                        std::is_move_constructible<T>>;

#define SOCKET_TEMPLATE template<class Socket, class T, \
                        typename = std::enable_if_t<socket_type<Socket>::value>, \
                        typename = std::enable_if_t<server_client<T, Socket>::value>>
#endif
}