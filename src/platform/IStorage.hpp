#pragma once
#include <string>
namespace mtg {
class IStorage {
public:
 virtual ~IStorage()=default;
 // A missing file is a successful read with an empty result.
 virtual bool load(std::string& data,std::string& error)=0;
 virtual bool save(const std::string& data,std::string& error)=0;
};
}
