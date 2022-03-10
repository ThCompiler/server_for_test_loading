#include "file_client.hpp"

#include <fstream>
#include <iostream>

static const char* GET_METHOD = "GET";
static const char* HEAD_METHOD = "HEAD";

static const char* STATUS_METHOD_NOT_ALLOWED = "HTTP/1.1 405 Method Not Allowed";
static const char* STATUS_NOT_FOUND = "HTTP/1.1 404 Not Found";
static const char* STATUS_FORBIDDEN = "HTTP/1.1 403 Forbidden";
static const char* STATUS_OK = "HTTP/1.1 200 OK";

static const char * divider = "\r\n";

std::string decode_url(const std::string &url) {
    std::string decoded_url;
    for (size_t i = 0; i < url.size(); i++) {
        if (url[i] == '%') {
            decoded_url += static_cast<char>(
                    strtoll(url.substr(i + 1, 2).c_str(), nullptr, 16)
            );
            i = i + 2;
        } else {
            decoded_url += url[i];
        }
    }
    return decoded_url;
}

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

// trim from both ends (in place)
static inline std::string trim(std::string s) {
    ltrim(s);
    rtrim(s);
    return s;
}

static const size_t client_chank_size = 1024;

using namespace file;

static std::string read_from_socket(bstcp::ISocket &socket, size_t chank_size) {
    tcp_data_t buffer(chank_size);

    bool status = socket.recv_from(buffer.data(), (int)chank_size);
    if (!status) {
        return "";
    }
    std::string res;
    res.insert(
            res.end(),
            std::make_move_iterator(buffer.begin()),
            std::make_move_iterator(buffer.end())
    );
    auto end = res.find('\0');
    if (end != std::string::npos) {
        res = res.substr(0, end);
    }

    return res;
}

static bool send_to_socket(bstcp::ISocket &socket, const std::string& data) {
    return socket.send_to(data.data(), (int)data.size());;
}

std::string read_file(const fs::path& path) {
    std::ifstream file(path,std::ios::in | std::ios::binary);
    std::stringstream fl;
    if (file.is_open()) {
        fl << file.rdbuf();
    }

    file.close();
    return fl.str();
}

std::string FileClient::_parse_request(std::string &data) {
    std::string response;

    auto end = data.find(divider);
    auto request = data.substr(0, end);

    end = request.find(' ');
    auto method = trim(request.substr(0, end));
    auto next_end = request.find('?', end + 1);
    if (next_end == std::string::npos) {
        next_end = request.find(' ', end + 1);
    }

    auto url = decode_url(trim(request.substr(end + 1 , next_end - 1 - end)));

    std::time_t now_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto time = std::string (std::ctime(&now_time));

    std::string headers = (std::string)"Connection: close" + divider;
    headers += (std::string)"Server: httpd" + divider;
    headers += (std::string)"Date: " + time.substr(0, time.size() - 1) + divider;

    if (method != GET_METHOD && method != HEAD_METHOD) {
        response += (std::string)STATUS_METHOD_NOT_ALLOWED + divider + headers + divider;
        return response;
    }

    auto res = _files.get_file(url);

    if (res.status == file_status::not_found) {
        response += (std::string)STATUS_NOT_FOUND + divider + headers + divider;
        return response;
    }

    if (res.status == file_status::forbidden) {
        response += (std::string)STATUS_FORBIDDEN + divider + headers + divider;
        return response;
    }

    auto body = read_file(res.path);
    if (body.empty()) {
        response += (std::string)STATUS_NOT_FOUND + divider + headers + divider;
        return response;
    }

    auto file_ex = res.path.extension().string();
    std::transform(file_ex.begin(), file_ex.end(), file_ex.begin(), tolower);
    auto content_type = Filesystem::encode_file_type(file_ex);
    if (content_type.empty()) {
        response += (std::string)STATUS_FORBIDDEN + divider + headers + divider;
        return response;
    }

    headers += "Content-Type: " + content_type + divider;
    headers += "Content-Length: " + std::to_string(body.size()) + divider;

    response += (std::string)STATUS_OK + divider + headers + divider;
    if (method == GET_METHOD) {
        response += body;
    }

    return response;
}


void FileClient::handle_request() {
    std::string data = read_from_socket(*this, client_chank_size);
    if (data.empty()) {
        return;
    }

   /* std::cout << "Client " << " send data [ " << data.size()
              << " bytes ]: \n" << (char *) data.data() << '\n';*/
    auto res = _parse_request(data);

    send_to_socket(*this, res);
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
