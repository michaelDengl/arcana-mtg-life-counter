#include "FileStorage.hpp"
#include <fstream>
#include <iterator>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
namespace mtg {
bool FileStorage::load(std::string& data,std::string& error) {
 try {
  data.clear();error.clear();if(!std::filesystem::exists(path_))return true;
  if(std::filesystem::file_size(path_)>4*1024*1024)throw std::runtime_error("Save file too large");
  std::ifstream file(path_,std::ios::binary);if(!file)throw std::runtime_error("Cannot read save file");
  data.assign(std::istreambuf_iterator<char>(file),{});if(file.bad())throw std::runtime_error("Save read failed");
  if(data.empty())throw std::runtime_error("Save file is empty");return true;
 }catch(const std::exception& e){error=e.what();return false;}
}
bool FileStorage::save(const std::string& data,std::string& error) {
 try {
  error.clear();if(!path_.parent_path().empty())std::filesystem::create_directories(path_.parent_path());
  auto temp=path_;temp+=".tmp";
  {std::ofstream file(temp,std::ios::binary|std::ios::trunc);file.write(data.data(),static_cast<std::streamsize>(data.size()));file.flush();if(!file)throw std::runtime_error("Cannot write save file");file.close();if(!file)throw std::runtime_error("Cannot finish save file");}
#ifdef _WIN32
  if(!MoveFileExW(temp.c_str(),path_.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot replace save file");
#else
  std::filesystem::rename(temp,path_);
#endif
  return true;
 }catch(const std::exception& e){error=e.what();return false;}
}
}
