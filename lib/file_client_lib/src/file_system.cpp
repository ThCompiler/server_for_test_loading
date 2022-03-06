#include "file_system.hpp"

namespace file {

std::size_t replace_all(std::string& inout, std::string_view what, std::string_view with) {
    std::size_t count{};
    for (std::string::size_type pos{};
         std::string::npos != (pos = inout.find(what.data(), pos, what.length()));
         pos += with.length(), ++count) {
        inout.replace(pos, what.length(), with.data(), with.length());
    }
    return count;
}

std::string Filesystem::encode_file_type(const std::string &extension) {
    if (extension == ".txt")
        return "text/txt";
    else if (extension == ".css")
        return "text/css";
    else if (extension == ".html")
        return "text/html";
    else if (extension == ".js")
        return "application/javascript";
    else if (extension == ".jpeg" || extension == ".jpg")
        return "image/jpeg";
    else if (extension == ".png")
        return "image/png";
    else if (extension == ".gif")
        return "image/gif";
    else if (extension == ".swf")
        return "application/x-shockwave-flash";
    return "";
}

requested_file_t Filesystem::get_file(const std::string& path_) const {
    auto path = path_;
    replace_all(path, "../", "");
    fs::path cur_path = _root_dir.string() + "/" + path;
    if (!fs::exists(cur_path)) {
        return requested_file_t{"", file_status::not_found};
    }

    if (is_directory(cur_path)) {
        bool allowed_file = false;
        for (const auto &file: fs::directory_iterator{cur_path}) {
            if (!file.is_directory() && fs::path(file).filename() == "index.html") {
                allowed_file = true;
                break;
            }
        }

        if (!allowed_file) {
            return requested_file_t{"", file_status::forbidden};
        }
        return requested_file_t{cur_path.append("/index.html"), file_status::correct};
    }
    return requested_file_t{cur_path, file_status::correct};
}

Filesystem::Filesystem(const std::string &root_dir)
    : _root_dir(root_dir) {}

}