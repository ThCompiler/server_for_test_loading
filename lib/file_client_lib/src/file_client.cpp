#include "file_client.hpp"

#include <fstream>
#include <iostream>

static const char* GET_METHOD = "GET";
static const char* HEAD_METHOD = "HEAD";

static const char* STATUS_METHOD_NOT_ALLOWED = "HTTP/1.1 405 Method Not Allowed\r\n";
static const char* STATUS_NOT_FOUND = "HTTP/1.1 404 Not Found\r\n";
static const char* STATUS_FORBIDDEN = "HTTP/1.1 403 Forbidden\r\n";
static const char* STATUS_OK = "HTTP/1.1 200 OK";

static const size_t client_chank_size = 1024;

using namespace file;

static std::string read_from_socket(bstcp::ISocket &socket, size_t chank_size) {
    tcp_data_t buffer(chank_size);

    bool status = socket.recv_from(buffer.data(), (int)chank_size);
    if (!status) {
        return "";
    }
    std::string res;

    while (status) {
        res.insert(
                res.end(),
                std::make_move_iterator(buffer.begin()),
                std::make_move_iterator(buffer.end())
        );
        auto end = res.find('\0');
        if (end != std::string::npos) {
            res = res.substr(0, end);
        }

        if(!socket.is_allow_to_read(1000)) {
            break;
        }

        buffer = tcp_data_t(chank_size);
        status = socket.recv_from(buffer.data(), (int)chank_size);
    }

    return res;
}

static bool send_to_socket(bstcp::ISocket &socket, const std::string& data, size_t chank_size) {
    bool res = true;
    for (size_t i = 0; (i < data.size()) && res; i += chank_size) {
        res = socket.send_to(data.data() + i, (int)std::min(chank_size, data.size() - i));
    }
    return res;
}

std::string read_file(const fs::path& path) {
    std::ifstream file(path);
    std::stringstream fl;
    if (file.is_open()) {
        fl << file.rdbuf();
    }

    file.close();
    return fl.str();
}

http::Request FileClient::_parse_request(std::string &data) {
    http::Request tmp(data);

    http::Request response;
    response.set_header("Connection", "close");
    response.set_header("Server", "httpd");
    std::time_t now_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    response.set_header("Date", std::ctime(&now_time));

    if (tmp.get_method() != GET_METHOD && tmp.get_method() != HEAD_METHOD) {
        response.set_url(STATUS_METHOD_NOT_ALLOWED);
        return response;
    }

    auto res = _files.get_file(tmp.get_url());

    if (res.status == file_status::not_found) {
        response.set_url(STATUS_NOT_FOUND);
        return response;
    }

    if (res.status == file_status::forbidden) {
        response.set_url(STATUS_FORBIDDEN);
        return response;
    }

    auto body = read_file(res.path);
    if (body.empty()) {
        response.set_url(STATUS_NOT_FOUND);
        return response;
    }

    auto content_type = Filesystem::encode_file_type(res.path.extension());
    if (content_type.empty()) {
        response.set_url(STATUS_FORBIDDEN);
        return response;
    }

    response.set_url(STATUS_OK);
    response.set_header("Content-Type", content_type);
    response.set_header("Content-Length", std::to_string(body.size()));

    if (tmp.get_method() == GET_METHOD) {
        response.set_body(body);
    }

    return response;
}


void FileClient::handle_request() {
    std::string data = read_from_socket(*this, client_chank_size);
    if (data.empty()) {
        return;
    }

    std::cout << "Client " << " send data [ " << data.size()
              << " bytes ]: \n" << (char *) data.data() << '\n';
    auto res = _parse_request(data);

    auto tmp = res.string();
    send_to_socket(*this, tmp, client_chank_size);
}

uint32_t FileClient::get_host() const {
    return _socket.get_host();
}

uint16_t FileClient::get_port() const {
    return _socket.get_port();
}

bstcp::SocketStatus FileClient::get_status() const {
    return _socket.get_status();
}

bstcp::SocketStatus FileClient::disconnect() {
    return _socket.disconnect();
}

bool FileClient::recv_from(void *buffer, int size) {
    return _socket.recv_from(buffer, size);
}

bool FileClient::send_to(const void *buffer, int size) const {
    return _socket.send_to(buffer, size);
}

SocketType FileClient::get_type() const {
    return SocketType::client_socket;
}

socket_t FileClient::get_socket() {
    return _socket.get_socket();
}

socket_addr_in FileClient::get_address() const {
    return _socket.get_address();
}

bool FileClient::is_allow_to_write(long timeout) const {
    return _socket.is_allow_to_write(timeout);
}

bool FileClient::is_allow_to_rwrite(long timeout) const {
    return _socket.is_allow_to_rwrite(timeout);
}

bool FileClient::is_allow_to_read(long timeout) const {
    return _socket.is_allow_to_read(timeout);
}

status FileClient::accept(const std::unique_ptr<ISocket> &server_socket) {
    return _socket.accept(server_socket);
}
