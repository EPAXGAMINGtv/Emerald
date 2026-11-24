#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

namespace test {

class TempDir {
public:
    explicit TempDir(std::string_view name) {
        path_ = std::filesystem::temp_directory_path()
            / std::filesystem::path("emerald_test_" + std::string(name));
        std::filesystem::remove_all(path_);
        std::filesystem::create_directories(path_);
    }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    TempDir(TempDir&& other) noexcept : path_(std::move(other.path_)) {
        other.path_.clear();
    }

    TempDir& operator=(TempDir&& other) noexcept {
        if (this != &other) {
            std::filesystem::remove_all(path_);
            path_ = std::move(other.path_);
            other.path_.clear();
        }
        return *this;
    }

    ~TempDir() {
        if (!path_.empty()) {
            std::filesystem::remove_all(path_);
        }
    }

    const std::filesystem::path& path() const noexcept {
        return path_;
    }

private:
    std::filesystem::path path_;
};

} // namespace test
