#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace file {

enum file_status: uint8_t {
    not_found   = 0,
    correct     = 1,
    forbidden   = 2
};

struct requested_file_t {
    fs::path path;
    file_status status;
};

class Filesystem {
  public:
    explicit Filesystem(const std::string& root_dir);

    [[nodiscard]] requested_file_t get_file(const std::string& path) const;

    static std::string encode_file_type(const std::string &extension);

  private:
    fs::path _root_dir;
};

};