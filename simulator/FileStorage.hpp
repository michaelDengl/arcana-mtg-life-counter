#pragma once
#include "platform/IStorage.hpp"
#include <filesystem>
namespace mtg {
class FileStorage final:public IStorage {
public:
 explicit FileStorage(std::filesystem::path path):path_(std::move(path)){}
 bool load(std::string& data,std::string& error)override;
 bool save(const std::string& data,std::string& error)override;
private:
 std::filesystem::path path_;
};
}
