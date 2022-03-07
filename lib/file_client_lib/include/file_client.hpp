#pragma once

#include <ostream>

#include "tcp_server_lib.hpp"
#include "file_system.hpp"

namespace file {

struct request_t;

struct FileClient : public bstcp::IServerClient {
  public:
    FileClient() = delete;

    explicit FileClient(BaseSocket &&socket)
            : _socket(std::move(socket))
            , _files(fs::current_path()) {}

    FileClient(const FileClient &) = delete;

    FileClient operator=(const FileClient &) = delete;

    FileClient(FileClient &&clt) noexcept
            : _socket(std::move(clt._socket))
            , _files(fs::current_path()) {}

    FileClient &operator=(const FileClient &&) = delete;

    ~FileClient() override = default;

    void handle_request() override;

    [[nodiscard]] uint32_t get_host() const override;

    [[nodiscard]] uint16_t get_port() const override;

    [[nodiscard]] bstcp::SocketStatus get_status() const override;

    bstcp::SocketStatus disconnect() final;

    bool recv_from(void *buffer, int size) override;

    bool send_to(const void *buffer, int size) const override;

    [[nodiscard]] SocketType get_type() const override;

    socket_t get_socket() override;

    [[nodiscard]] socket_addr_in get_address() const;

    [[nodiscard]] bool is_allow_to_read(long timeout) const override;

    [[nodiscard]] bool is_allow_to_write(long timeout) const override;

    [[nodiscard]] bool is_allow_to_rwrite(long timeout) const override;

  private:

    status accept(const std::unique_ptr<ISocket>& server_socket) override;

    std::string _parse_request(std::string &data);

    BaseSocket _socket;

    file::Filesystem _files;
};

}